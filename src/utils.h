#ifndef UTILS_H
#define UTILS_H

#include <inttypes.h>
#include <avr/pgmspace.h>

#define MAX(x, y) ((x) > (y) ? (x) : (y))
#define MIN(x, y) ((x) < (y) ? (x) : (y))
#define CLAMP(x, min, max) MAX(MIN((x), (max)), (min))

//! Safe int8_t add (no overflow and underflow)
static inline int8_t safe_add_8(int8_t a, int8_t b)
{
    int8_t r = (uint8_t)a + (uint8_t)b;
    uint8_t asgn = (a & (1 << 7)) != 0;
    if (!asgn && r < b) return INT8_MAX; 
    else if (asgn && r > b) return INT8_MIN;
    else return r;
}

//! Safe int16_t add (no overflow and underflow)
static inline int16_t safe_add_16(int16_t a, int16_t b)
{
    int16_t r = (uint16_t)a + (uint16_t)b;
    uint8_t asgn = (a & (1 << 15)) != 0;
    if (!asgn && r < b) return INT16_MAX; 
    else if (asgn && r > b) return INT16_MIN;
    else return r;
}

//! Safe int32_t add (no overflow and underflow)
static inline int32_t safe_add_32(int32_t a, int32_t b)
{
    int32_t r = (uint32_t)a + (uint32_t)b;
    uint8_t asgn = (a & (1ul << 31)) != 0;
    if (!asgn && r < b) return INT32_MAX; 
    else if (asgn && r > b) return INT32_MIN;
    else return r;
}

//! Read uint16_t from delta-compressed uint8_t array in PROGMEM
static inline uint16_t pgm_read_delta_word(const uint8_t *ptr, uint16_t n)
{
	uint16_t offset = n & 0xfffe; // offset = (n / 2) * 3
	offset += n >> 1;
	uint16_t base_word = pgm_read_word(ptr + offset);

	if (n & 1)
	{
		uint8_t delta_byte = pgm_read_byte(ptr + offset + 2);
		return base_word + delta_byte;
	}
	else
	{
		return base_word;
	}
}

#endif
