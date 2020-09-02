#ifndef PPG_H
#define PPG_H

#include <avr/pgmspace.h>
#include <inttypes.h>
#include "ppg_data.h"

//! This would be 64, but we don't need the additional 3 waveforms that PPG provides
#define PPG_DEFAULT_WAVETABLE_SIZE 61

//! Contains currently used wavetable
typedef struct ppg_wavetable_entry
{
	const uint8_t *ptr_l;
	const uint8_t *ptr_r;
	uint8_t factor;
	uint8_t is_key;
	uint16_t _padding;
} ppg_wavetable_entry;

//! Returns a pointer to the wave with certain index (that can later be passed to get_waveform_sample())
static inline const uint8_t *ppg_get_waveform_pointer(uint8_t index)
{
	return ppg_waveforms_data + (index << 6);
}

//! This shall act as pgm_read_byte()
static inline uint8_t ppg_get_waveform_sample(const uint8_t *ptr, uint8_t sample)
{
	return pgm_read_byte(ptr + sample);
}

//! Reads sample from a 64-byte waveform buffer based on 16-bit phase value
static inline uint8_t ppg_get_waveform_sample_by_phase(const uint8_t *ptr, uint16_t phase2b)
{
	// This phase ranges 0-127
	uint8_t phase = ((uint8_t*) &phase2b)[1] >> 1;
	uint8_t half_select = phase & 64;
	phase &= 63; // Poor man's modulo 64

	// Waveform mirroring
	if (half_select)
		return ppg_get_waveform_sample(ptr, phase);
	else
		return 255u - ppg_get_waveform_sample(ptr, 63u - phase);
}

//! Reads a single sample based on a wavetable entry
static inline uint16_t ppg_get_wavetable_sample(const ppg_wavetable_entry *e, uint16_t phase2b)
{
	uint8_t sample_l = ppg_get_waveform_sample_by_phase(e->ptr_l, phase2b);
	uint8_t sample_r = ppg_get_waveform_sample_by_phase(e->ptr_r, phase2b);
	uint8_t factor = e->factor;
	uint16_t mix_l = (256 - factor) * sample_l;
	uint16_t mix_r = factor * sample_r;
	return mix_l + mix_r;
}

extern const uint8_t *ppg_load_wavetable(ppg_wavetable_entry *entries, uint8_t wavetable_size, const uint8_t *data);
extern const uint8_t *ppg_load_wavetable_n(ppg_wavetable_entry *entries, uint8_t wavetable_size, const uint8_t *data, uint8_t index);

#endif
