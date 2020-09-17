#include "midi.h"
#include <stddef.h>
#include <inttypes.h>
#include <string.h>
#include <avr/pgmspace.h>
#include "midi_program.h"

void midi_init(midi_status *midi, uint8_t voice_count)
{
	midi->dlim = 0;
	midi->dcnt = 0;
	midi->status = 0;
	midi->channel = 0;
	midi->voice_count = voice_count;

	midi->program = 0;
	midi->pitchbend = 8192;
	midi->reset = 0;

	memset(midi->voices, 0, sizeof(midi_voice) * MIDI_MAX_VOICES);
	memset(midi->control, 0, 128);
}

/**
	Finds an empty voice slot and uses it or overwrites the oldest
	active slot
*/
static inline void midi_note_on(midi_status *midi, uint8_t note, uint8_t velocity)
{
	uint8_t oldest_empty_slot = MIDI_MAX_VOICES;
	uint8_t oldest_active_slot = 0;
	int8_t oldest_empty = -1;
	int8_t oldest_active = -1;

	for (uint8_t i = 0; i < midi->voice_count; i++)
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
	for (uint8_t i = 0; i < midi->voice_count; i++)
		midi->voices[i].age++;

	// Always prefer empty slot over reuse
	uint8_t slot = oldest_empty_slot == MIDI_MAX_VOICES ? oldest_active_slot : oldest_empty_slot;
	midi->voices[slot].gate = MIDI_GATE_ON_BIT | MIDI_GATE_TRIG_BIT;
	midi->voices[slot].note = note;
	midi->voices[slot].velocity = velocity;
	midi->voices[slot].age = 0;	
}

static inline void midi_note_off(midi_status *midi, uint8_t note)
{
	for (uint8_t i = 0; i < MIDI_MAX_VOICES; i++)
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
					break;

				// Program change
				case 0x40:
					midi_program_load(midi, midi->dbuf[0]);
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

void midi_clear_trig_bits(midi_status *midi)
{
	for (uint8_t i = 0; i < MIDI_MAX_VOICES; i++)
		midi->voices[i].gate &= ~MIDI_GATE_TRIG_BIT;
}