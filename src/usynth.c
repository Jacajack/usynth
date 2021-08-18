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
	MIDI data ring buffer - written in interrupt, read in the main loop
*/
static volatile uint8_t midi_buffer[256];
static volatile uint8_t midi_wcnt = 0;

/**
	Main interrupt - sends a new sample to the DAC
*/
static volatile uint16_t dac_data = 0;
static volatile uint8_t dac_sent = 0;
ISR(TIMER1_COMPB_vect)
{
	// CS is automatically set low, but LDAC needs to be forced high
	TCCR1A |= (1 << COM1A0);
	TCCR1C |= (1 << FOC1A);
	TCCR1A &= ~(1 << COM1A1);

	const uint16_t mcp4921_conf = MCP4921_SHDN_BIT | MCP4921_GAIN_BIT | MCP4921_VREF_BUF_BIT;
	uint16_t data = mcp4921_conf | (dac_data >> 4);
	
	// Send the data via SPI
	SPDR = data >> 8;
	while (!(SPSR & (1 << SPIF)));
	SPDR = data;
	while (!(SPSR & (1 << SPIF)));
	
	// Force compare event to set CS high
	TCCR1A |= (1 << COM1B0);
	TCCR1C |= (1 << FOC1B);
	TCCR1A &= ~(1 << COM1B1);
	
	// Check incoming USART data and buffer it
	// No need for a while loop here - this interrupt
	// is frequent enough
	if (UCSR0A & (1 << RXC0))
		midi_buffer[midi_wcnt++] = UDR0;

	// Set sent flag, so the main loop can work again
	dac_sent = 1;
}

// Synth globals
static usynth_voice voices[2];
static filter1pole filter;
static int8_t filter_cutoff;

// MIDI
static midi_status midi;
static uint8_t poly_mode = 1;
static uint8_t midi_voice_offset = 0;

// For mapping MIDI CC values to uint8_t and int8_t 
#define MIDI_CTL(x) (midi.control[(x)])
#define MIDI_CTL_S8(x) (((int8_t)(MIDI_CTL((x))) - 64) << 1)
#define MIDI_CTL_U8(x) ((MIDI_CTL((x))) << 1)
#define MIDI_CTL_BOOL(x) (MIDI_CTL(x) != 0)

/**
	Updates voice state based on MIDI control parameters (part 1)
	\param cc_set determines from which MIDI CC set to update
*/
static inline void voice_update_cc_1(usynth_voice *v, uint8_t cc_set) __attribute__((always_inline));
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
	v->eg_pitch_int = (int8_t)MIDI_CTL(MIDI_EG_PITCH_INT(cc_set)) - 64;
	v->lfo_pitch_int = (int8_t)MIDI_CTL(MIDI_LFO_PITCH_INT(cc_set)) - 64;

	v->lfo.step = MIDI_CTL_U8(MIDI_LFO_RATE(cc_set)) << 1;
	v->lfo.waveform = MIDI_CTL(MIDI_LFO_WAVE(cc_set));
	v->lfo.fade_step = pgm_read_word(env_table + MIDI_CTL(MIDI_LFO_FADE(cc_set)));

	// Loads wavetable when it changes
	if (v->wavetable_number != MIDI_CTL(MIDI_OSC_WAVETABLE(cc_set)))
	{
		v->wavetable_number = MIN(MIDI_CTL(MIDI_OSC_WAVETABLE(cc_set)), PPG_WAVETABLE_COUNT - 1);

		// Write back to the MIDI controls array, so the wavetable
		// is not reloaded over and over if it's out of range
		MIDI_CTL(MIDI_OSC_WAVETABLE(cc_set)) = v->wavetable_number;

		ppg_osc_load_wavetable(&v->osc, v->wavetable_number);
	}
}

/**
	Updates voice's gate - retriggers LFO and EGs and propagates gate value to them.
	\param cc_set determines from which MIDI CC set to update
	\param midi_voice determines polyphony voice ID to use
*/
static inline void voice_update_gate(usynth_voice *v, uint8_t cc_set, uint8_t midi_gate)
{
	// Executed once on keypress
	if (midi_gate & MIDI_GATE_TRIG_BIT)
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

	v->amp_eg.gate = midi_gate;
	v->mod_eg.gate = midi_gate;
	v->lfo.gate = midi_gate;
}

/**
	Updates voice's frequency based on MIDI note and all modulation sources
	\param cc_set determines from which MIDI CC set to update
	\param midi_voice determines polyphony voice ID to use
*/
static inline void voice_update_note(usynth_voice *v, uint8_t cc_set, uint8_t midi_note) __attribute__((always_inline));
static inline void voice_update_note(usynth_voice *v, uint8_t cc_set, uint8_t midi_note)
{
	int16_t note = (int16_t)midi_note + MIDI_CTL(MIDI_OSC_PITCH(cc_set)) - 64 - 4; // Subtract 4 here intead of subtracting 128 later
	note <<= 5; // Now we operate on 100/32 cents
	note += (int16_t)(midi.pitchbend >> 7) + MIDI_CTL(MIDI_OSC_DETUNE(cc_set));
	note += (v->eg_pitch_int * (int8_t)(v->mod_eg.output >> 9)) >> 3;
	note += (v->lfo_pitch_int * (int8_t)(v->lfo.output >> 8)) >> 4;
	
	// Equivalent of CLAMP(note, 0, 128 * 32 - 1)
	note = note < 0 ? 0 : note;
	note &= 4095;
	v->osc.phase_step = pgm_read_delta_word(notes_table, note);
}

/**
	Updates currently used waveform
*/
static inline void voice_update_mod(usynth_voice *v)
{
	int16_t mod = v->base_wave;
	mod += (v->eg_mod_int * (int8_t)(v->mod_eg.output >> 9)) >> 5;
	mod += (v->lfo_mod_int * (int8_t)(v->lfo.output >> 8)) >> 6;

	// The -128 - 127 range is mapped to 0 - 64
	mod = 32 + (mod >> 2);
	v->osc.wave = CLAMP(mod, 0, PPG_DEFAULT_WAVETABLE_SIZE - 1);
}

/**
	Updates global/common synth state
*/
static inline void update_global_1(void)
{
	// Resets phase of all LFOs
	if (MIDI_CTL(MIDI_LFO_RESET))
	{
		MIDI_CTL(MIDI_LFO_RESET) = 0;
		usynth_lfo_sync(&voices[0].lfo);
		usynth_lfo_sync(&voices[1].lfo);	
	}

	// Handle ping requests
	if (MIDI_CTL(MIDI_PING))
	{
		// Transmit one byte of ping response
		while (!(UCSR0A & (1 << UDRE0)));
		UDR0 = MIDI_CTL(MIDI_PING);
		MIDI_CTL(MIDI_PING) = 0;
	}

	// Filter control
	filter_cutoff = MIDI_CTL(MIDI_CUTOFF) >> 1;

	// Clear 'triggered' gate bits
	midi_clear_trig_bits(&midi);
}

/**
	Updates global state - handles mono/poly switching and cluster operation
*/
static inline void update_global_2(void)
{
	// Mono/poly and cluster logic
	uint8_t cluster_size = CLAMP(MIDI_CTL(MIDI_CLUSTER_SIZE), 1, MIDI_MAX_VOICES / 2);
	uint8_t cluster_id = MIN(MIDI_CTL(MIDI_CLUSTER_ID), cluster_size - 1);
	poly_mode = MIDI_CTL(MIDI_POLY) != 0;
	midi.voice_count = (poly_mode + 1) * cluster_size;
	midi_voice_offset = (poly_mode + 1) * cluster_id;
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
	
	/*
		TIMER 1 - CTC mode, F_CPU

		- Clear OC1B (CS) on compare
		- Clear OC1A (LDAC) on compare
		- COMP1B ISR active

		Event:    COMPB       FORCE
		__   _______|    ISR    |__________
		CS          |___________|

		Event:      FORCE           COMPA
		____          |_______________|
		LDAC _________|               |____
	*/
	TCCR1A = (1 << COM1A1) | (1 << COM1B1);
	TCCR1B = (1 << WGM12) | (1 << CS10);
	TIMSK1 = (1 << OCIE1B);
 	OCR1A = F_CPU / F_SAMPLE - 1;
 	OCR1B = 4; // Determines LDAC pulse width (must be over 100ns)
	
	// USART0 - MIDI, 8 bit data, 1 bit stop, no parity
	UBRR0 = F_CPU / 16 / MIDI_BAUD - 1;
	UCSR0B = (1 << RXEN0) | (1 << TXEN0);
	UCSR0C = (1 << UCSZ00) | (1<< UCSZ01);
	
	// -------------- HW init done

	// Force wavetable reload by storing a fake number
	voices[0].wavetable_number = 255;
	voices[1].wavetable_number = 255;

	midi_init(&midi, 2);
	midi_program_load(&midi, 0);
	MIDI_CTL(MIDI_CLUSTER_SIZE) = 1;
	MIDI_CTL(MIDI_CLUSTER_ID) = 0;

	sei();

	// The main loop
	uint8_t load_balancer_cnt = 0;
	uint8_t midi_rcnt = 0;
	while (1)
	{
		/*
			Distribute workload evenly across each 21 samples
			
			31250 / 8 / 28000 * 20 = ~2.92 which means that reading
			3 MIDI data bytes in the loop is sufficient
		*/
		switch (load_balancer_cnt++)
		{
			// Process MIDI byte
			case 0:
				if (midi_rcnt != midi_wcnt) midi_process_byte(&midi, midi_buffer[midi_rcnt++], 0);
				break;

			// Process MIDI byte
			case 1:
				if (midi_rcnt != midi_wcnt) midi_process_byte(&midi, midi_buffer[midi_rcnt++], 0);
				break;

			// Process MIDI byte
			case 2:
				if (midi_rcnt != midi_wcnt) midi_process_byte(&midi, midi_buffer[midi_rcnt++], 0);
				break;

			// Update from MIDI (1/2) (Voice 0)
			case 3:
				voice_update_cc_1(&voices[0], 0);
				break;

			// Update from MIDI (2/2) (Voice 0)
			case 4:
				voice_update_cc_2(&voices[0], 0);
				break;

			// Update from MIDI (1/2) (Voice 1)
			case 5:
				if (poly_mode)
					voice_update_cc_1(&voices[1], 0);
				else
					voice_update_cc_1(&voices[1], 1);
				break;

			// Update from MIDI (2/2) (Voice 1)
			case 6:
				voice_update_cc_2(&voices[1], !poly_mode);
				break;

			// Update control parameters 0, 1
			case 7:
				voice_update_gate(&voices[0], 0, midi.voices[midi_voice_offset].gate);
				voice_update_gate(&voices[1], !poly_mode, midi.voices[midi_voice_offset + poly_mode].gate);
				break;
			
			// Update frequency (Voice 1)
			case 8:
				voice_update_note(&voices[0], 0, midi.voices[midi_voice_offset].note);
				break;

			// Update frequency (Voice 2)
			case 9:
				if (poly_mode)
					voice_update_note(&voices[1], 0, midi.voices[midi_voice_offset + poly_mode].note);
				else
					voice_update_note(&voices[1], 1, midi.voices[midi_voice_offset + poly_mode].note);
				break;

			// Update globals (1/2)
			case 10:	
				update_global_1();
				break;

			// Update globals (2/2)
			case 11:
				update_global_2();
				break;
				
			// AMP EG 0
			case 12:
				usynth_eg_update(&voices[0].amp_eg);
				break;

			// AMP EG 1
			case 13:
				usynth_eg_update(&voices[1].amp_eg);
				break;

			// MOD EG 0 
			case 14:
				usynth_eg_update(&voices[0].mod_eg);
				break;

			// MOD EG 1
			case 15:
				usynth_eg_update(&voices[1].mod_eg);
				break;

			// LFO 0
			case 16:
				usynth_lfo_update(&voices[0].lfo);
				break;

			// LFO 1
			case 17:
				usynth_lfo_update(&voices[1].lfo);
				break;

			// Update modulation 0
			case 18:
				voice_update_mod(&voices[0]);
				break;

			// Update modulation 1
			case 19:
				voice_update_mod(&voices[1]);
				break;

			// LEDs and counter reset
			case 20:
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
		}

		ppg_osc_update(&voices[0].osc);
		ppg_osc_update(&voices[1].osc);

		// Mixing
		uint16_t x0, x1;
		MUL_U16_U16_16H(x0, voices[0].osc.output, voices[0].amp_eg.output);
		MUL_U16_U8_16H(x0, x0, midi.voices[midi_voice_offset].velocity << 1);
		MUL_U16_U16_16H(x1, voices[1].osc.output, voices[1].amp_eg.output);
		MUL_U16_U8_16H(x1, x1, midi.voices[midi_voice_offset + poly_mode].velocity << 1);

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
