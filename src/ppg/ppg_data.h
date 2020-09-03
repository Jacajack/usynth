#ifndef PPG_DATA_AVR_H
#define PPG_DATA_AVR_H

#include <inttypes.h>
#include <avr/pgmspace.h>

#define PPG_WAVETABLE_COUNT 29

extern const uint8_t ppg_wavetable_data[] PROGMEM;
extern const uint8_t ppg_waveforms_data[] PROGMEM;
extern const uint16_t ppg_wavetable_offsets[PPG_WAVETABLE_COUNT] PROGMEM;

#endif
