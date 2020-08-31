#include "ppg.h"
#include <inttypes.h>
#include <string.h>

/**
	Load a wavetable stored in PPG Wave 2.2 format into an array of wavetable_entry structs of size wavetable_size
	Returns a pointer to the next wavetable
*/
const uint8_t *load_wavetable(struct wavetable_entry *entries, uint8_t wavetable_size, const uint8_t *data)
{
	// Wipe the wavetable
	memset(entries, 0, wavetable_size * sizeof(struct wavetable_entry));

	// The fist byte is ignored
	data++;

	// Read wavetable entries up to size - 1
	uint8_t waveform, pos;
	do
	{
		waveform = pgm_read_byte(data++);
		pos = pgm_read_byte(data++);

		entries[pos].ptr_l = get_waveform_pointer(waveform);
		entries[pos].ptr_r = NULL;
		entries[pos].factor = 0;
		entries[pos].is_key = 1;
	}
	while (pos < wavetable_size - 1);

	// Now, generate interpolation coefficients
	const struct wavetable_entry *el = NULL, *er = NULL;
	for (uint8_t i = 0; i < wavetable_size; i++)
	{
		// If the current entry contains a key-wave
		if (entries[i].is_key)
		{
			// Write both pointers in case the right key waveform is not found
			el = er = &entries[i];

			// Look for the next key-wave
			for (uint8_t j = i + 1; j < wavetable_size; j++)
			{
				if (entries[j].is_key)
				{
					er = &entries[j];
					break;
				}
			}
		}

		// Total distance between known key-waves and distance from the left one
		uint8_t distance_total = er - el;
		uint8_t distance_l = &entries[i] - el;

		entries[i].ptr_l = el->ptr_l;
		entries[i].ptr_r = er->ptr_l;

		// We have to avoid division by 0 for the last slot
		if (distance_total != 0)
			entries[i].factor = (65535 / distance_total * distance_l) >> 8;
		else
			entries[i].factor = 0;
	}

	// Return pointer to the next wavetable
	return data;
}

//! Loads n-th requested wavetable from binary format
//! Not very efficient, but it doesn't need to be.
//! \see load_wavetable()
const uint8_t *load_wavetable_n(struct wavetable_entry *entries, uint8_t wavetable_size, const uint8_t *data, uint8_t index)
{
	for ( uint8_t i = 0; i < index + 1; i++ )
		data = load_wavetable(entries, wavetable_size, data);
	return data;
}



