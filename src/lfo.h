#ifndef USYNTH_LFO
#define USYNTH_LFO

#include <inttypes.h>

#define USYNTH_LFO_TRIANGLE 0
#define USYNTH_LFO_SQUARE   1

#define USYNTH_LFO_DIR_BIT  (1 << 0)

typedef struct usynth_lfo
{
	int16_t step;
	uint8_t waveform;
	uint16_t fade_step;

	uint16_t fade;
	int16_t value;
	int16_t output;
	uint8_t status;
	uint8_t gate;
} usynth_lfo;

static inline void usynth_lfo_sync(usynth_lfo *lfo)
{
	lfo->value = 0;
	lfo->output = 0;
	lfo->status = 0;
}

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

	// Fade update
	if (lfo->gate)
	{
		uint16_t tmp = lfo->fade + lfo->fade_step;
		lfo->fade = tmp < lfo->fade ? UINT16_MAX : tmp;
	}

	// Waveform select
	if (lfo->waveform == USYNTH_LFO_TRIANGLE)
		MUL_S16_U16_16H(lfo->output, lfo->value, lfo->fade);
	else
		lfo->output = lfo->value > 0 ? (lfo->fade >> 1) : -(lfo->fade >> 1);
}

#endif