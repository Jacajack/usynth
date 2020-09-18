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
	Loads MIDI preset
*/
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
			// If the highest bit is set, the next CC is set as well
			// Handy when setting CC pairs
			midi->control[param & 0x7f] = value & 0x7f;
			if (param & MIDI_BOTH)
				midi->control[(param & 0x7f) + 1] = value & 0x7f;
		}
	}

	// On succesful load
	if (found)
		midi->program = id;
}