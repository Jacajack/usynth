#ifndef FILTER_H
#define FILTER_H

#include <inttypes.h>

// Some DSP type aliases
typedef int32_t integrator;
typedef integrator filter1pole;

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

// A 32-bit overflow/underflow-safe digital integrator
static inline integrator integrator_feed(integrator *i, int32_t x)
{
	return *i = safe_add_32(*i, x);
}

//! A 1 pole filter based on the above integrator
//! \see integrator
static inline int16_t filter1pole_feed(filter1pole *f, int8_t k, int16_t x)
{
	integrator_feed(f, ((x - (*f >> 8)) * k));
	return *f >> 8;
}

#endif