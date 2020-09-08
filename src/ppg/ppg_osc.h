#ifndef PPG_OSC_H
#define PPG_OSC_H

#include "ppg.h"
#include "ppg_data.h"

#include "../config.h"

typedef struct ppg_osc
{
	ppg_wavetable_entry wt[PPG_DEFAULT_WAVETABLE_SIZE];

	uint16_t phase;
	uint16_t phase_step;
	uint16_t output;
	uint8_t wave;
} ppg_osc;

static inline void ppg_osc_load_wavetable(ppg_osc *osc, uint8_t index)
{
	ppg_load_wavetable_n(osc->wt, PPG_DEFAULT_WAVETABLE_SIZE, ppg_wavetable_data, index);
}

static inline void ppg_osc_update(ppg_osc *osc)
{
	osc->phase += osc->phase_step;
	osc->output = ppg_get_wavetable_sample(&osc->wt[osc->wave], osc->phase);
}

#endif