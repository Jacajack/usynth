#ifndef FILTER_H
#define FILTER_H

#include <inttypes.h>
#include "utils.h"

// Some DSP type aliases
typedef int32_t integrator;
typedef integrator filter1pole;

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