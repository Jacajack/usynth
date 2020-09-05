#include "usynth.h"
#include <avr/io.h>
#include <avr/interrupt.h>
#include <inttypes.h>
#include <stddef.h>

#include "config.h"
#include "midi.h"
#include "ppg/ppg_osc.h"
#include "eg.h"
#include "data/notes_table.h"
#include "data/mod_table.h"
#include "data/env_table.h"
#include "mul.h"
#include "utils.h"
#include "filter.h"
#include "lfo.h"

#ifndef F_CPU
#error F_CPU is not defined!
#endif

#ifndef F_SAMPLE
#error F_SAMPLE is not defined!
#endif

#ifndef MIDI_BAUD
#warning MIDI_BAUD is not defined! Assuming default 31250
#define MIDI_BAUD 31250
#endif

#define LOAD_BALANCER_MAX 15

/**
	Main interrupt - sends a new sample to the DAC
*/
volatile uint16_t dac_data = 0;
volatile uint8_t dac_sent = 0;
ISR(TIMER1_COMPB_vect)
{
	// CS is automatically set low
	const uint16_t mcp4921_conf = MCP4921_SHDN_BIT | MCP4921_GAIN_BIT | MCP4921_VREF_BUF_BIT;
	uint16_t data = mcp4921_conf | (dac_data >> 4);
	
	// Send the data via SPI
	SPDR = data >> 8;
	while (!(SPSR & (1 << SPIF)));
	SPDR = data;
	while (!(SPSR & (1 << SPIF)));
	
	// Force compare event to set CS high
	TCCR1A = (1 << COM1B0) | (1 << COM1B1);
	TCCR1C |= (1 << FOC1B);
	TCCR1A = (1 << COM1B1);
	
	// Set sent flag, so the main loop can work again
	dac_sent = 1;
}

// Synth globals
static midi_status midi;
static ppg_osc_bank osc_bank;
static usynth_eg_bank amp_eg_bank;
static usynth_eg_bank mod_eg_bank;
static usynth_lfo lfo;
static int8_t base_wave;
static int8_t eg_mod_int;
static int8_t lfo_mod_int;
static int8_t filter_k;
static uint8_t wavetable_number = 255;
static uint8_t midi_buffer[8];
static uint8_t midi_len = 0;

/**
	Updates synth state based on MIDI control parameters
*/
static inline void update_controls(void)
{
	base_wave = MIDI_CONTROL_TO_S8(midi.control[MIDI_BASE_WAVE]);
	eg_mod_int = MIDI_CONTROL_TO_S8(midi.control[MIDI_MOD_EG_INT]);
	lfo_mod_int = MIDI_CONTROL_TO_S8(midi.control[MIDI_MOD_LFO_INT]);

	amp_eg_bank.attack  = pgm_read_word(env_table + MIDI_CONTROL_TO_U8(midi.control[MIDI_AMP_ATTACK]));
	amp_eg_bank.sustain = MIDI_CONTROL_TO_U8(midi.control[MIDI_AMP_SUSTAIN]);
	amp_eg_bank.release = pgm_read_word(env_table + MIDI_CONTROL_TO_U8(midi.control[MIDI_AMP_RELEASE]));
	amp_eg_bank.sustain_enabled = midi.control[MIDI_AMP_ASR];

	mod_eg_bank.attack  = pgm_read_word(env_table + MIDI_CONTROL_TO_U8(midi.control[MIDI_MOD_ATTACK]));
	mod_eg_bank.sustain = MIDI_CONTROL_TO_U8(midi.control[MIDI_MOD_SUSTAIN]);
	mod_eg_bank.release = pgm_read_word(env_table + MIDI_CONTROL_TO_U8(midi.control[MIDI_MOD_RELEASE]));
	mod_eg_bank.sustain_enabled = midi.control[MIDI_MOD_ASR];

	lfo.step = MIDI_CONTROL_TO_U8(midi.control[MIDI_LFO_RATE]) << 1;
	lfo.waveform = midi.control[MIDI_LFO_WAVE];

	filter_k = MIDI_CONTROL_TO_S8(midi.control[MIDI_CUTOFF]);

	if (wavetable_number != midi.control[MIDI_WAVETABLE])
	{
		wavetable_number = midi.control[MIDI_WAVETABLE];
		wavetable_number = wavetable_number >= PPG_WAVETABLE_COUNT ? PPG_WAVETABLE_COUNT - 1 : wavetable_number;

		// Write back to the MIDI controls array, so the wavetable
		// is not reloaded over and over if it's out of range
		midi.control[MIDI_WAVETABLE] = wavetable_number;

		ppg_osc_bank_load_wavetable(&osc_bank, wavetable_number);
	}
}

/**
	Updates parameters of oscillators based on modulation.
	Also responsible for retrigerring EGs
*/
static inline void update_params(uint8_t id)
{
	int16_t mod = base_wave;
	int8_t eg_mod = (eg_mod_int * (int8_t)(mod_eg_bank.eg[id].output >> 9)) >> 7;
	int8_t lfo_mod = (lfo_mod_int * (int8_t)(lfo.output >> 8)) >> 7;
	mod += eg_mod;
	mod += lfo_mod;

	// Clamp
	if (mod < INT8_MIN) mod = INT8_MIN;
	else if (mod > INT8_MAX) mod = INT8_MAX;

	if (midi.voices[id].gate & MIDI_GATE_TRIG_BIT)
	{
		midi.voices[id].gate = MIDI_GATE_ON_BIT;
		amp_eg_bank.eg[id].status = USYNTH_EG_IDLE;
		amp_eg_bank.eg[id].value = 0;
		mod_eg_bank.eg[id].status = USYNTH_EG_IDLE;
		mod_eg_bank.eg[id].value = 0;
		osc_bank.osc[id].phase = 0;
	}

	osc_bank.osc[id].phase_step = pgm_read_word(midi_notes + midi.voices[id].note);
	osc_bank.osc[id].wave = pgm_read_byte(mod_table + (uint8_t)(int8_t)mod);
	amp_eg_bank.eg[id].gate = midi.voices[id].gate;
	mod_eg_bank.eg[id].gate = midi.voices[id].gate;
}

int main(void)
{
	// LEDs
	DDRD |= (1 << LED_1_PIN) | (1 << LED_2_PIN) | (1 << LED_3_PIN);
	
	// 10 MHz SPI Master, DAC IO
	PORTB |= (1 << LDAC_PIN) | (1 << CS_PIN);
	DDRB |= (1 << LDAC_PIN) | (1 << CS_PIN) | (1 << MOSI_PIN) | (1 << SCK_PIN);
	SPCR = (1 << SPE) | (1 << MSTR);
	SPSR = (1 << SPI2X);
	
	// LDAC tied low, CS is used as word clock
	PORTB &= ~(1 << LDAC_PIN);
	
	/*
		TIMER 1 - CTC mode, F_CPU

		- Clear OC1B (CS) on compare
		- COMP1B ISR active
	*/
	TCCR1B = (1 << WGM12) | (1 << CS10);
	TIMSK1 = (1 << OCIE1B);
 	OCR1A = F_CPU / F_SAMPLE - 1;
 	OCR1B = 0;
	
	// Set CS high and enable 'clear on compare'
	TCCR1A = (1 << COM1B0) | (1 << COM1B1);
	TCCR1C |= (1 << FOC1B);
	TCCR1A = (1 << COM1B1);

	// USART0 - MIDI, 8 bit data, 1 bit stop, no parity
	UBRR0 = F_CPU / 16 / MIDI_BAUD - 1;
	UCSR0B = (1 << RXEN0) | (1 << TXEN0);
	UCSR0C = (1 << UCSZ00) | (1<< UCSZ01);
	
	// -------------- HW init done

	midi_init(&midi);
	midi_program_load(&midi, 0);

	sei();

	// The main loop
	uint8_t load_balancer_cnt = LOAD_BALANCER_MAX;
	while (1)
	{
		// Check incoming USART data and buffer it
		if (UCSR0A & (1 << RXC0))
			midi_buffer[midi_len++] = UDR0;

		// Distribute workload evenly across each 16 samples
		switch (--load_balancer_cnt)
		{
			// Process all received MIDI commands
			case 9:
				for (uint8_t i = 0; i < midi_len; i++)
					midi_process_byte(&midi, midi_buffer[i], 0);
				midi_len = 0;
				break;

			// Update from MIDI
			case 8:
				update_controls();
				break;

			// Modulation / EG retriggering 0
			case 7:
				update_params(0);
				break;
			
			// Modulation / EG retriggering 1
			case 6:
				update_params(1);
				break;

			// AMP EG 0
			case 5:
				usynth_eg_bank_update_eg(&amp_eg_bank, 0);
				break;

			// AMP EG 1
			case 4:
				usynth_eg_bank_update_eg(&amp_eg_bank, 1);
				break;

			// MOD EG 0 
			case 3:
				usynth_eg_bank_update_eg(&mod_eg_bank, 0);
				break;

			// MOD EG 1
			case 2:
				usynth_eg_bank_update_eg(&mod_eg_bank, 1);
				break;

			// LEDs, LFO and counter reset
			case 1:
				usynth_lfo_update(&lfo);
				break;

			// LEDs and counter reset
			case 0:
				if (amp_eg_bank.eg[0].output >> 8)
					PORTD |= (1 << LED_RED_PIN);
				else
					PORTD &= ~(1 << LED_RED_PIN);

				if (amp_eg_bank.eg[1].output >> 8)
					PORTD |= (1 << LED_YLW_PIN);
				else
					PORTD &= ~(1 << LED_YLW_PIN);

				load_balancer_cnt = LOAD_BALANCER_MAX;
				break;

			// Empty steps
			default:
				break;
		}

		ppg_osc_bank_update(&osc_bank);

		uint16_t x0, x1;
		MUL_U16_U16_16H(x0, osc_bank.osc[0].output, amp_eg_bank.eg[0].output);
		MUL_U16_U8_16H(x0, x0, midi.voices[0].velocity << 1);
		MUL_U16_U16_16H(x1, osc_bank.osc[1].output, amp_eg_bank.eg[1].output);
		MUL_U16_U8_16H(x1, x1, midi.voices[1].velocity << 1);

		int16_t x = (x0 >> 1) + (x1 >> 1) - 32768;
		static filter1pole fi = 0;
		x = filter1pole_feed(&fi, midi.control[MIDI_CUTOFF], x);

		dac_data = x + 32768;
		
		// Wait for the 'sent' flag and clear it
		while (!dac_sent)
		{
			if (load_balancer_cnt == midi.control[MIDI_WORKLOAD_CHANNEL]) PORTD |= (1 << LED_GRN_PIN);
		}
		PORTD &= ~(1 << LED_GRN_PIN);
		dac_sent = 0;
	}
}
