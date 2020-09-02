#ifndef PPG_OSC_H
#define PPG_OSC_H

#include "ppg.h"
#include "ppg_data.h"

#include "../config.h"

#ifndef PPG_OSC_BANK_SIZE
#error PPG_OSC_BANK_SIZE not defined
#endif

typedef struct ppg_osc
{
	uint16_t phase;
	uint16_t phase_step;
	uint16_t output;
	uint8_t wave;
} ppg_osc;

typedef struct ppg_osc_bank
{
	ppg_wavetable_entry wt[PPG_DEFAULT_WAVETABLE_SIZE];
	ppg_osc osc[PPG_OSC_BANK_SIZE];
} ppg_osc_bank;

static inline void ppg_osc_bank_load_wavetable(ppg_osc_bank *bank, uint8_t index)
{
	ppg_load_wavetable_n(bank->wt, PPG_DEFAULT_WAVETABLE_SIZE, ppg_wavetable_data, index);
}

static inline void ppg_osc_bank_update(ppg_osc_bank *bank)
{
	for (uint8_t i = 0; i < PPG_OSC_BANK_SIZE; i++)
	{
		bank->osc[i].phase += bank->osc[i].phase_step;
		bank->osc[i].output = ppg_get_wavetable_sample(&bank->wt[bank->osc[i].wave], bank->osc[i].phase);
	}
}

#endif