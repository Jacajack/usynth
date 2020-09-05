#ifndef NOTES_TABLE_H
#define NOTES_TABLE_H

#include <inttypes.h>
#include <avr/pgmspace.h>

extern const uint8_t notes_table[] PROGMEM;

/**
	Returns phase step for requested note
*/
static inline uint16_t get_note_phase_step(uint16_t note)
{
	uint16_t offset = (note >> 1) * 3;
	uint16_t base_word = pgm_read_word(notes_table + offset);

	if (note & 1)
	{
		uint8_t delta_byte = pgm_read_byte(notes_table + offset + 2);
		return base_word + delta_byte;
	}
	else
	{
		return base_word;
	}
}

#endif 
