#include <stddef.h>
#include <inttypes.h>
#include <string.h>
#include <avr/pgmspace.h>
#include "midi.h"

void midi_init(midi_status *midi)
{
	midi->dlim = 0;
	midi->dcnt = 0;
	midi->status = 0;
	midi->channel = 0;

	midi->program = 0;
	midi->pitchbend = 16384;
	midi->reset = 0;

	memset(midi->voices, 0, sizeof(midi_voice));
	memset(midi->control, 0, 128);
}

/**
	Finds an empty voice slot and uses it or overwrites the oldest
	active slot
*/
static inline void midi_note_on(midi_status *midi, uint8_t note, uint8_t velocity)
{
	uint8_t oldest_empty_slot = MIDI_VOICES;
	uint8_t oldest_active_slot = 0;
	int8_t oldest_empty = -1;
	int8_t oldest_active = -1;

	for (uint8_t i = 0; i < MIDI_VOICES; i++)
	{
		int8_t age = midi->voices[i].age;

		if (midi->voices[i].gate)
		{
			if (age > oldest_active)
			{
				oldest_active_slot = i;
				oldest_active = midi->voices[i].age;
			}
		}
		else 
		{
			if (age > oldest_empty)
			{
				oldest_empty_slot = i;
				oldest_empty = age;
			}
		}
	}

	// Increment age of all voices
	for (uint8_t i = 0; i < MIDI_VOICES; i++)
		midi->voices[i].age++;

	// Always prefer empty slot over reuse
	uint8_t slot = oldest_empty_slot == MIDI_VOICES ? oldest_active_slot : oldest_empty_slot;
	midi->voices[slot].gate = MIDI_GATE_ON_BIT | MIDI_GATE_TRIG_BIT;
	midi->voices[slot].note = note;
	midi->voices[slot].velocity = velocity;
	midi->voices[slot].age = 0;	
}

static inline void midi_note_off(midi_status *midi, uint8_t note)
{
	for (uint8_t i = 0; i < MIDI_VOICES; i++)
		if (midi->voices[i].note == note)
			midi->voices[i].gate = 0;
}

void midi_process_byte(midi_status *midi, uint8_t byte, uint8_t channel)
{
	uint8_t dlim, dcnt, status;

	// Null pointer check
	if (midi == NULL) return;

	// Get configuration from the struct
	dlim = midi->dlim;
	dcnt = midi->dcnt;
	status = midi->status;

	// Handle synthesizer reset
	if (byte == 0xff) midi->reset = 1;

	if (byte & (1 << 7)) // Handle status bytes
	{
		// Extract information from status byte
		status = byte & 0x70;
		dcnt = dlim = 0;
		midi->channel = byte & 0x0f;

		// Check data length for each MIDI command
		switch (status)
		{
			// Note on
			case 0x10:
				dlim = 2;
				break;

			// Note off
			case 0x00:
				dlim = 2;
				break;

			// Controller change
			case 0x30:
				dlim = 2;
				break;

			// Program change
			case 0x40:
				dlim = 1;
				break;

			// Pitch
			case 0x60:
				dlim = 2;
				break;

			// Uknown command
			default:
				dlim = 0;
				break;

		}
	}
	else if (midi->channel == channel) // Handle data bytes
	{
		// Data byte
		midi->dbuf[dcnt++] = byte;

		// Interpret command
		if (dcnt >= dlim)
		{
			switch (status)
			{
				// Note on
				case 0x10:
					midi_note_on(midi, midi->dbuf[0], midi->dbuf[1]);
					break;

				// Note off
				case 0x00:
					midi_note_off(midi, midi->dbuf[0]);
					break;

				// Controller change
				case 0x30:
					midi->control[midi->dbuf[0]] = midi->dbuf[1];
					if (midi->control_change_handler)
						midi->control_change_handler(midi->dbuf[0], midi->dbuf[1]);
					break;

				// Program change
				case 0x40:
					midi_program_load(midi, midi->dbuf[0]);
					if (midi->program_change_handler)
						midi->program_change_handler(midi->program);
					break;

				// Pitch
				case 0x60:
					midi->pitchbend = midi->dbuf[0] | (midi->dbuf[1] << 7);
					break;

				default:
					break;
			}

			dcnt = 0;
		}

	}

	// Write config back to the struct
	midi->dlim = dlim;
	midi->dcnt = dcnt;
	midi->status = status;
}

/**
	Default MIDI programs
*/
#define MIDI_PARAM_PROGRAM_BEGIN     0xff
#define MIDI_PARAM_PROGRAM_TABLE_END 0xfe
#define MIDI_PROGRAM_BEGIN(id) {MIDI_PARAM_PROGRAM_BEGIN, (id)}
#define MIDI_PROGRAM_TABLE_END() {MIDI_PARAM_PROGRAM_TABLE_END, 0}
static const midi_program_data midi_program_table[] PROGMEM =
{
	// The defaults loaded before loading any program
	MIDI_PROGRAM_BEGIN(0xff),
	{MIDI_WAVETABLE,    0},
	{MIDI_BASE_WAVE,    64},
	
	{MIDI_AMP_ATTACK,   0},
	{MIDI_AMP_SUSTAIN,  127},
	{MIDI_AMP_RELEASE,  0},
	{MIDI_AMP_ASR,      1},
	
	{MIDI_MOD_ATTACK,   0},
	{MIDI_MOD_SUSTAIN,  127},
	{MIDI_MOD_RELEASE,  0},
	{MIDI_MOD_ASR,      1},
	{MIDI_MOD_EG_INT,   64},
	
	{MIDI_CUTOFF,       64},
	{MIDI_RESONANCE,    0},

	{MIDI_LFO_RATE,     64},
	{MIDI_LFO_WAVE,     0},
	{MIDI_MOD_LFO_INT,  64},


	// Program 0 - some kind of marimba
	MIDI_PROGRAM_BEGIN(0),
	{MIDI_WAVETABLE,    2},
	{MIDI_BASE_WAVE,    0},
	
	{MIDI_AMP_ATTACK,   15},
	{MIDI_AMP_SUSTAIN,  127},
	{MIDI_AMP_RELEASE,  91},
	{MIDI_AMP_ASR,      1},
	
	{MIDI_MOD_ATTACK,   0},
	{MIDI_MOD_SUSTAIN,  90},
	{MIDI_MOD_RELEASE,  76},
	{MIDI_MOD_ASR,      0},
	{MIDI_MOD_EG_INT,  127},


	// Program 1 - Morpher
	MIDI_PROGRAM_BEGIN(1),
	{MIDI_WAVETABLE,    9},
	{MIDI_BASE_WAVE,    102},
	
	{MIDI_AMP_ATTACK,   15},
	{MIDI_AMP_SUSTAIN,  127},
	{MIDI_AMP_RELEASE,  108},
	{MIDI_AMP_ASR,      1},
	
	{MIDI_MOD_ATTACK,   0},
	{MIDI_MOD_SUSTAIN,  127},
	{MIDI_MOD_RELEASE,  114},
	{MIDI_MOD_ASR,      0},
	{MIDI_MOD_EG_INT,   0},

	{MIDI_CUTOFF,       104},


	MIDI_PROGRAM_TABLE_END(),
};

void midi_program_load(midi_status *midi, uint8_t id)
{
	// Load defaults before loading anything
	if (id != 0xff)
		midi_program_load(midi, 0xff);

	const midi_program_data *ptr;
	uint8_t param = 0;
	uint8_t value = 0;
	uint8_t found = 0;

	for (ptr = midi_program_table; param != MIDI_PARAM_PROGRAM_TABLE_END; ptr++)
	{
		param = pgm_read_byte(&ptr->param);
		value = pgm_read_byte(&ptr->value);
		
		// If on desired program label, start writing
		// If already writing, break
		if (param == MIDI_PARAM_PROGRAM_BEGIN)
		{
			if (found)
				break;
			else if (value == id)
				found = 1;
		}
		else if (found)
		{
			midi->control[param & 0x7f] = value & 0x7f;
		}
	}

	// On succesful load
	if (found)
	{
		midi->program = id;

		// Emit controller change message
		// so the synth logic can be updated
		if (midi->control_change_handler)
			midi->control_change_handler(param, value);
	}
}