#ifndef USYNTH_LFO
#define USYNTH_LFO

#include <inttypes.h>
#include "config.h"

#ifndef USYNTH_LFO_BANK_SIZE
#error USYNTH_LFO_BANK_SIZE not defined!
#endif

#define USYNTH_LFO_TRIANGLE 0
#define USYNTH_LFO_SQUARE   1

#define USYNTH_LFO_DIR_BIT  (1 << 0)

typedef struct usynth_lfo
{
	uint16_t fade;
	int16_t value;
	int16_t output;
	uint8_t status;
	uint8_t gate;
} usynth_lfo;

typedef struct usynth_lfo_bank
{
	int16_t step;
	uint8_t waveform;
	uint16_t fade_step;

	usynth_lfo lfo[USYNTH_LFO_BANK_SIZE];
} usynth_lfo_bank;

static inline void usynth_lfo_sync(usynth_lfo *lfo)
{
	lfo->value = 0;
	lfo->output = 0;
	lfo->status = 0;
}

static inline void usynth_lfo_bank_update_lfo(usynth_lfo_bank *bank, uint8_t id)
{
	usynth_lfo *lfo = &bank->lfo[id];

	// Triangle
	if (lfo->status & USYNTH_LFO_DIR_BIT)
	{
		int16_t tmp = INT16_MAX - bank->step;
		if (tmp < lfo->value)
		{
			lfo->value = INT16_MAX - (lfo->value - tmp);
			lfo->status &= ~USYNTH_LFO_DIR_BIT;
		}
		else
			lfo->value += bank->step;
	}
	else
	{
		int16_t tmp = INT16_MIN + bank->step;
		if (tmp > lfo->value)
		{
			lfo->value = INT16_MIN - (lfo->value - tmp);
			lfo->status |= USYNTH_LFO_DIR_BIT;
		}
		else
			lfo->value -= bank->step;
	}

	// Fade update
	if (lfo->gate)
	{
		uint16_t tmp = lfo->fade + bank->fade_step;
		lfo->fade = tmp < lfo->fade ? UINT16_MAX : tmp;
	}

	// Waveform select
	if (bank->waveform == USYNTH_LFO_TRIANGLE)
		MUL_S16_U16_16H(lfo->output, lfo->value, lfo->fade);
	else
		lfo->output = lfo->value > 0 ? (lfo->fade >> 1) : -(lfo->fade >> 1);
}

static inline void usynth_lfo_bank_update(usynth_lfo_bank *bank)
{
	for (uint8_t i = 0; i < USYNTH_LFO_BANK_SIZE; i++)
		usynth_lfo_bank_update_lfo(bank, i);
}

#endif