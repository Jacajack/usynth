#ifndef USYNTH_LFO
#define USYNTH_LFO

#include <inttypes.h>

#define USYNTH_LFO_TRIANGLE 0
#define USYNTH_LFO_SQUARE   1

#define USYNTH_LFO_DIR_BIT  (1 << 0)

typedef struct usynth_lfo
{
	int16_t value;
	int16_t output;
	int16_t step;
	uint8_t status;
	uint8_t waveform;
} usynth_lfo;

static inline void usynth_lfo_update(usynth_lfo *lfo)
{
	// Triangle
	if (lfo->status & USYNTH_LFO_DIR_BIT)
	{
		int16_t tmp = INT16_MAX - lfo->step;
		if (tmp < lfo->value)
		{
			lfo->value = INT16_MAX - (lfo->value - tmp);
			lfo->status &= ~USYNTH_LFO_DIR_BIT;
		}
		else
			lfo->value += lfo->step;
	}
	else
	{
		int16_t tmp = INT16_MIN + lfo->step;
		if (tmp > lfo->value)
		{
			lfo->value = INT16_MIN - (lfo->value - tmp);
			lfo->status |= USYNTH_LFO_DIR_BIT;
		}
		else
			lfo->value -= lfo->step;
	}

	// Waveform select
	if (lfo->waveform == USYNTH_LFO_TRIANGLE)
		lfo->output = lfo->value;
	else
		lfo->output = lfo->value > 0 ? INT16_MAX : INT16_MIN;
}

#endif