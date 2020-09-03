#ifndef UTILS_H
#define UTILS_H

#include <inttypes.h>

#define SAFE_ADD_INT8(res, x) if ((x) > 0) {if (127 - (x) < (res)) (res) = 127; else (res) += (x);} else {if (-128 - (x) > (res)) (res) = -128; else (res) += (x);}
#define MIDI_CONTROL_TO_S8(x) (((int8_t)(x) - 64) << 1)
#define MIDI_CONTROL_TO_U8(x) ((x) << 1)

//! Safe int8_t add (no overflow and underflow)
static inline int8_t safe_add_8(int8_t a, int8_t b)
{
	if (a > 0 && b > INT8_MAX - a)
		return INT8_MAX;
	else if (a < 0 && b < INT8_MIN - a)
		return INT8_MIN;
	return a + b;
}

//! Safe int16_t add (no overflow and underflow)
static inline int16_t safe_add_16(int16_t a, int16_t b)
{
	if (a > 0 && b > INT16_MAX - a)
		return INT16_MAX;
	else if (a < 0 && b < INT16_MIN - a)
		return INT16_MIN;
	return a + b;
}

//! Safe int32_t add (no overflow and underflow)
static inline int32_t safe_add_32(int32_t a, int32_t b)
{
	if (a > 0 && b > INT32_MAX - a)
		return INT32_MAX;
	else if (a < 0 && b < INT32_MIN - a)
		return INT32_MIN;
	return a + b;
}

#endif