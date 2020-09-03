#include "usynth.h"
#include <avr/io.h>
#include <avr/interrupt.h>
#include <inttypes.h>

#include "config.h"
#include "notes.h"
#include "midi.h"
#include "ppg/ppg_osc.h"
#include "eg.h"
#include "mod_table.h"
#include "env_table.h"
#include "mul.h"
#include "utils.h"
#include "filter.h"

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
static int8_t base_wave = 0;
static int8_t eg_mod_int = 127;
static int8_t filter_k = 64;

static void set_midi_program(uint8_t program)
{
	ppg_osc_bank_load_wavetable(&osc_bank, program);
}

static void midi_control_change(uint8_t index, uint8_t value)
{
	base_wave = MIDI_CONTROL_TO_S8(midi.control[MIDI_BASE_WAVE]);
	eg_mod_int = MIDI_CONTROL_TO_S8(midi.control[MIDI_MOD_EG_INT]);

	amp_eg_bank.attack  = pgm_read_word(env_table + MIDI_CONTROL_TO_U8(midi.control[MIDI_AMP_ATTACK]));
	amp_eg_bank.sustain = MIDI_CONTROL_TO_U8(midi.control[MIDI_AMP_SUSTAIN]);
	amp_eg_bank.release = pgm_read_word(env_table + MIDI_CONTROL_TO_U8(midi.control[MIDI_AMP_RELEASE]));
	amp_eg_bank.sustain_enabled = midi.control[MIDI_AMP_ASR];

	mod_eg_bank.attack  = pgm_read_word(env_table + MIDI_CONTROL_TO_U8(midi.control[MIDI_MOD_ATTACK]));
	mod_eg_bank.sustain = MIDI_CONTROL_TO_U8(midi.control[MIDI_MOD_SUSTAIN]);
	mod_eg_bank.release = pgm_read_word(env_table + MIDI_CONTROL_TO_U8(midi.control[MIDI_MOD_RELEASE]));
	mod_eg_bank.sustain_enabled = midi.control[MIDI_MOD_ASR];

	filter_k = MIDI_CONTROL_TO_S8(midi.control[MIDI_CUTOFF]);
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
	midi.program_change_handler = set_midi_program;
	midi.control_change_handler = midi_control_change;

	// MIDI defaults
	midi.control[MIDI_BASE_WAVE] = 64;
	midi.control[MIDI_MOD_EG_INT] = 64;
	
	midi.control[MIDI_AMP_ATTACK] = 0;
	midi.control[MIDI_AMP_SUSTAIN] = 127;
	midi.control[MIDI_AMP_RELEASE] = 79;
	midi.control[MIDI_AMP_ASR] = 1;

	midi.control[MIDI_MOD_ATTACK] = 0;
	midi.control[MIDI_MOD_SUSTAIN] = 127;
	midi.control[MIDI_MOD_RELEASE] = 79;
	midi.control[MIDI_MOD_ASR] = 1;

	midi.control[MIDI_CUTOFF] = 64;

	set_midi_program(0);
	midi_control_change(0, 0);
	
	sei();

	// The main loop
	uint8_t slow_cnt = 0;
	while (1)
	{
		// Check incoming USART data and process it
		if (UCSR0A & (1 << RXC0))
			midi_process_byte(&midi, UDR0, 0);

		// Distribute work evenly across each 16 samples
		switch (--slow_cnt)
		{
			// Modulation and control parameter setup
			case 2:
				for (uint8_t i = 0; i < USYNTH_VOICES; i++)
				{
					int8_t mod = base_wave;
					int8_t eg_mod = (eg_mod_int * (int8_t)(mod_eg_bank.eg[i].output >> 9)) >> 7;
					SAFE_ADD_INT8(mod, eg_mod);

					if (midi.voices[i].gate & MIDI_GATE_TRIG_BIT)
					{
						midi.voices[i].gate = MIDI_GATE_ON_BIT;
						amp_eg_bank.eg[i].status = USYNTH_EG_IDLE;
						amp_eg_bank.eg[i].value = 0;
						mod_eg_bank.eg[i].status = USYNTH_EG_IDLE;
						mod_eg_bank.eg[i].value = 0;
						osc_bank.osc[i].phase = 0;
					}

					osc_bank.osc[i].phase_step = pgm_read_word(midi_notes + midi.voices[i].note);
					osc_bank.osc[i].wave = pgm_read_byte(mod_table + (uint8_t) mod);
					amp_eg_bank.eg[i].gate = midi.voices[i].gate;
					mod_eg_bank.eg[i].gate = midi.voices[i].gate;
				}
				break;

			// EG update 
			case 1:
				for (uint8_t i = 0; i < USYNTH_VOICES; i++)
				{
					usynth_eg_bank_update(&amp_eg_bank);
					usynth_eg_bank_update(&mod_eg_bank);
				}
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

				slow_cnt = 15;
				break;

			// Empty steps
			default:
				break;
		}

		ppg_osc_bank_update(&osc_bank);

		uint16_t x0, x1, x2;
		MUL_U16_U16_16H(x0, osc_bank.osc[0].output, amp_eg_bank.eg[0].output);
		MUL_U16_U16_16H(x1, osc_bank.osc[1].output, amp_eg_bank.eg[1].output);
		MUL_U16_U16_16H(x2, osc_bank.osc[2].output, amp_eg_bank.eg[2].output);
		int16_t x = (x0 >> 2) + (x1 >> 2) + (x2 >> 2) - 32768;

		static filter1pole fi = 0;
		x = filter1pole_feed(&fi, midi.control[MIDI_CUTOFF], x);

		dac_data = x + 32768;
		
		// Wait for the 'sent' flag and clear it
		while (!dac_sent)
		{
			if (slow_cnt == 14) PORTD |= (1 << LED_GRN_PIN);
		}
		PORTD &= ~(1 << LED_GRN_PIN);
		dac_sent = 0;
	}
}
