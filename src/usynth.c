#include "usynth.h"
#include <avr/io.h>
#include <avr/interrupt.h>
#include <inttypes.h>
#include <stddef.h>

#include "utils.h"
#include "mul.h"
#include "midi.h"
#include "midi_program.h"
#include "midi_cc.h"
#include "ppg/ppg_osc.h"
#include "eg.h"
#include "lfo.h"
#include "filter.h"
#include "data/notes_table.h"
#include "data/env_table.h"

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
	MIDI data buffer capacity. Has to be a power of two.
*/
#ifndef MIDI_BUFFER_SIZE
#define MIDI_BUFFER_SIZE 32
#endif

/**
	MIDI data buffer - written in interrupt, read in the main loop
	\warning MIDI_BUFFER_SIZE has to be a power of 2
*/
static volatile uint8_t midi_buffer[MIDI_BUFFER_SIZE];
static volatile uint8_t midi_len = 0;

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
	
	// Check incoming USART data and buffer it
	// No need for a while loop here - this interrupt
	// is frequent enough
	midi_len &= MIDI_BUFFER_SIZE - 1;
	if (UCSR0A & (1 << RXC0))
		midi_buffer[midi_len++] = UDR0;

	// Force compare event to set CS high
	TCCR1A = (1 << COM1B0) | (1 << COM1B1);
	TCCR1C |= (1 << FOC1B);
	TCCR1A = (1 << COM1B1);
	
	// Set sent flag, so the main loop can work again
	dac_sent = 1;
}

typedef struct usynth_voice
{
	ppg_osc osc;
	usynth_eg amp_eg;
	usynth_eg mod_eg;
	usynth_lfo lfo;
	int8_t base_wave;
	int8_t eg_mod_int;
	int8_t lfo_mod_int;
	uint8_t wavetable_number;
} usynth_voice;


// Synth globals
static usynth_voice voices[2];
static filter1pole filter;
static int8_t filter_cutoff;

// MIDI
static midi_status midi;
static uint8_t poly_mode = 1;

// For mapping MIDI CC values to uint8_t and int8_t 
#define MIDI_CTL(x) (midi.control[(x)])
#define MIDI_CTL_S8(x) (((int8_t)(MIDI_CTL((x))) - 64) << 1)
#define MIDI_CTL_U8(x) ((MIDI_CTL((x))) << 1)

/**
	Updates voice state based on MIDI control parameters (part 1)
	\param cc_set determines from which MIDI CC set to update
*/
static inline void voice_update_cc_1(usynth_voice *v, uint8_t cc_set)
{
	v->base_wave = MIDI_CTL_S8(MIDI_OSC_BASE_WAVE(cc_set));
	v->eg_mod_int = MIDI_CTL_S8(MIDI_EG_MOD_INT(cc_set));
	v->lfo_mod_int = MIDI_CTL_S8(MIDI_LFO_MOD_INT(cc_set));

	v->amp_eg.attack  = pgm_read_word(env_table + MIDI_CTL(MIDI_AMP_A(cc_set)));
	v->amp_eg.sustain = MIDI_CTL_U8(MIDI_AMP_S(cc_set));
	v->amp_eg.release = pgm_read_word(env_table + MIDI_CTL(MIDI_AMP_R(cc_set)));
	v->amp_eg.sustain_enabled = MIDI_CTL(MIDI_AMP_ASR(cc_set));

	v->mod_eg.attack  = pgm_read_word(env_table + MIDI_CTL(MIDI_EG_A(cc_set)));
	v->mod_eg.sustain = MIDI_CTL_U8(MIDI_EG_S(cc_set));
	v->mod_eg.release = pgm_read_word(env_table + MIDI_CTL(MIDI_EG_R(cc_set)));
	v->mod_eg.sustain_enabled = MIDI_CTL(MIDI_EG_ASR(cc_set));
}

/**
	Updates voice state based on MIDI control parameters (part 2)
	\param cc_set determines from which MIDI CC set to update
*/
static inline void voice_update_cc_2(usynth_voice *v, uint8_t cc_set)
{
	v->lfo.step = MIDI_CTL_U8(MIDI_LFO_RATE(cc_set)) << 1;
	v->lfo.waveform = MIDI_CTL(MIDI_LFO_WAVE(cc_set));
	v->lfo.fade_step = pgm_read_word(env_table + MIDI_CTL(MIDI_LFO_FADE(cc_set)));

	// Loads wavetable when it changes
	if (v->wavetable_number != MIDI_CTL(MIDI_OSC_WAVETABLE(cc_set)))
	{
		v->wavetable_number = MIDI_CTL(MIDI_OSC_WAVETABLE(cc_set));
		v->wavetable_number = v->wavetable_number >= PPG_WAVETABLE_COUNT ? PPG_WAVETABLE_COUNT - 1 : v->wavetable_number;

		// Write back to the MIDI controls array, so the wavetable
		// is not reloaded over and over if it's out of range
		MIDI_CTL(MIDI_OSC_WAVETABLE(cc_set)) = v->wavetable_number;

		ppg_osc_load_wavetable(&v->osc, v->wavetable_number);
	}
}

/**
	Updates voice's oscillator.
	Also responsible for retrigerring EGs/LFOs
	\param cc_set determines from which MIDI CC set to update
	\param midi_voice determines polyphony voice ID to use
*/
static inline void voice_update(usynth_voice *v, uint8_t cc_set, uint8_t midi_voice)
{
	// Executed once on keypress
	if (midi.voices[midi_voice].gate & MIDI_GATE_TRIG_BIT)
	{
		// Reset EGs
		v->amp_eg.status = USYNTH_EG_IDLE;
		v->amp_eg.value = 0;
		v->mod_eg.status = USYNTH_EG_IDLE;
		v->mod_eg.value = 0;
		
		// Reset oscillator
		v->osc.phase = 0;

		// Reset LFO
		v->lfo.fade = 0;
		if (MIDI_CTL(MIDI_LFO_SYNC))
			usynth_lfo_sync(&v->lfo);
	}

	int16_t note = (int16_t)midi.voices[midi_voice].note + MIDI_CTL(MIDI_OSC_PITCH(cc_set)) - 64;
	note <<= 5;
	note += (int16_t)(midi.pitchbend >> 7) - 64 + MIDI_CTL(MIDI_OSC_DETUNE(cc_set)) - 64;
	
	// Clamp
	if (note < 0) note = 0;
	else if (note >= 128 * 32) note = 128 * 32 - 1;

	v->osc.phase_step = 1 + pgm_read_delta_word(notes_table, note);
	v->amp_eg.gate = midi.voices[midi_voice].gate;
	v->mod_eg.gate = midi.voices[midi_voice].gate;
	v->lfo.gate = midi.voices[midi_voice].gate;
}

/**
	Updates currently used waveform
*/
static inline void voice_update_mod(usynth_voice *v)
{
	int16_t mod = v->base_wave;
	int8_t eg_mod = (v->eg_mod_int * (int8_t)(v->mod_eg.output >> 9)) >> 7; // TODO Fix modulation intensity to cover 256
	int8_t lfo_mod = (v->lfo_mod_int * (int8_t)(v->lfo.output >> 8)) >> 7;
	mod += eg_mod;
	mod += lfo_mod;
	mod = 32 + (mod >> 2);

	// Clamp
	if (mod < 0) mod = 0;
	else if (mod > PPG_DEFAULT_WAVETABLE_SIZE - 1) mod = PPG_DEFAULT_WAVETABLE_SIZE - 1;
	v->osc.wave = mod;
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

	// Force wavetable reload by storing a fake number
	voices[0].wavetable_number = 255;
	voices[1].wavetable_number = 255;

	poly_mode = 1;
	midi_init(&midi, 2);
	midi_program_load(&midi, 0);

	sei();

	// The main loop
	uint8_t load_balancer_cnt = 0;
	while (1)
	{
		// Distribute workload evenly across each 16 samples
		switch (load_balancer_cnt++)
		{
			// Process all received MIDI commands
			case 0:
				for (uint8_t i = 0; i < midi_len; i++)
					midi_process_byte(&midi, midi_buffer[i], 0);
				midi_len = 0;
				break;

			// Update from MIDI (1/2) (Voice 0)
			case 1:
				voice_update_cc_1(&voices[0], 0);
				break;

			// Update from MIDI (2/2) (Voice 0)
			case 2:
				voice_update_cc_2(&voices[0], 0);
				break;

			// Update from MIDI (1/2) (Voice 1)
			case 3:
				if (MIDI_CTL(MIDI_POLY))
					voice_update_cc_1(&voices[1], 0);
				else
					voice_update_cc_1(&voices[1], 1);
				break;

			// Update from MIDI (2/2) (Voice 1)
			case 4:
				voice_update_cc_2(&voices[1], !MIDI_CTL(MIDI_POLY));
				break;

			// Update control parameters 0
			case 5:
				voice_update(&voices[0], 0, 0);
				break;
			
			// Update control parameters 1
			case 6:
				voice_update(&voices[1], !MIDI_CTL(MIDI_POLY), !!MIDI_CTL(MIDI_POLY));

				// Filter control
				filter_cutoff = MIDI_CTL(MIDI_CUTOFF) >> 1;

				// Resets phase of all LFOs
				if (MIDI_CTL(MIDI_LFO_RESET))
				{
					MIDI_CTL(MIDI_LFO_RESET) = 0;
					usynth_lfo_sync(&voices[0].lfo);
					usynth_lfo_sync(&voices[1].lfo);	
				}
				break;

			// AMP EG 0
			case 7:
				// Clear 'triggered' gate bits
				poly_mode = MIDI_CTL(MIDI_POLY);
				midi.voice_count = MIDI_CTL(MIDI_POLY) + 1;
				midi_clear_trig_bits(&midi);

				usynth_eg_update(&voices[0].amp_eg);
				break;

			// AMP EG 1
			case 8:
				usynth_eg_update(&voices[1].amp_eg);
				break;

			// MOD EG 0 
			case 9:
				usynth_eg_update(&voices[0].mod_eg);
				break;

			// MOD EG 1
			case 10:
				usynth_eg_update(&voices[1].mod_eg);
				break;

			// LFO 0
			case 11:
				usynth_lfo_update(&voices[0].lfo);
				break;

			// LFO 1
			case 12:
				usynth_lfo_update(&voices[1].lfo);
				break;

			// Update modulation 0
			case 13:
				voice_update_mod(&voices[0]);
				break;

			// Update modulation 1
			case 14:
				voice_update_mod(&voices[1]);
				break;

			// LEDs and counter reset
			case 15:
				if (voices[0].amp_eg.output >> 8)
					PORTD |= (1 << LED_RED_PIN);
				else
					PORTD &= ~(1 << LED_RED_PIN);

				if (voices[1].amp_eg.output >> 8)
					PORTD |= (1 << LED_YLW_PIN);
				else
					PORTD &= ~(1 << LED_YLW_PIN);

				load_balancer_cnt = 0;
				break;

			// Empty steps
			default:
				break;
		}

		ppg_osc_update(&voices[0].osc);
		ppg_osc_update(&voices[1].osc);

		// Mixing
		uint16_t x0, x1;
		MUL_U16_U16_16H(x0, voices[0].osc.output, voices[0].amp_eg.output);
		MUL_U16_U8_16H(x0, x0, midi.voices[0].velocity << 1);
		MUL_U16_U16_16H(x1, voices[1].osc.output, voices[1].amp_eg.output);
		MUL_U16_U8_16H(x1, x1, midi.voices[MIDI_CTL(MIDI_POLY)].velocity << 1);

		// Filter
		int16_t x = (x0 >> 1) + (x1 >> 1) - 32768;
		x = filter1pole_feed(&filter, filter_cutoff, x);

		// Output sample
		dac_data = x + 32768;
		
		// Wait for the 'sent' flag and clear it
		while (!dac_sent)
		{
			if (load_balancer_cnt == MIDI_CTL(MIDI_DEBUG_CHANNEL)) PORTD |= (1 << LED_GRN_PIN);
		}
		PORTD &= ~(1 << LED_GRN_PIN);
		dac_sent = 0;
	}
}
