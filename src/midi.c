#include <stddef.h>
#include <inttypes.h>
#include <string.h>
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
	midi->voices[slot].gate = 1;
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
					break;

				// Program change
				case 0x40:
					midi->program = midi->dbuf[0];
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
