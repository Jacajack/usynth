#ifndef EG_H
#define EG_H

#include "config.h"

#ifndef USYNTH_EG_BANK_SIZE
#error USYNTH_EG_BANK_SIZE not defined!
#endif

#define USYNTH_EG_IDLE    0
#define USYNTH_EG_ATTACK  1
#define USYNTH_EG_SUSTAIN 2
#define USYNTH_EG_RELEASE 3

typedef struct usynth_eg
{
	uint8_t gate;
	uint8_t status;
	uint16_t value;
	uint16_t output;
} usynth_eg;

typedef struct usynth_eg_bank
{
	// Common parameters
	uint16_t attack;
	uint16_t sustain;
	uint16_t release;
	
	// EG channels
	usynth_eg eg[USYNTH_EG_BANK_SIZE];
} usynth_eg_bank;


static inline uint16_t usynth_eg_bank_update(usynth_eg_bank *bank)
{
	for (uint8_t i = 0; i < USYNTH_EG_BANK_SIZE; i++)
	{
		usynth_eg *eg = &bank->eg[i];

		switch (eg->status)
		{
			case USYNTH_EG_IDLE:
				if (eg->gate) eg->status = USYNTH_EG_ATTACK;
				break;
					
			case USYNTH_EG_ATTACK:
				if (!eg->gate) eg->status = USYNTH_EG_RELEASE;
				else
				{
					if (UINT16_MAX - bank->attack < eg->value)
					{
						eg->value = UINT16_MAX;
						eg->status = USYNTH_EG_SUSTAIN;
					}
					else
					{
						eg->value += bank->attack;
					}
				}
				break;
				
			case USYNTH_EG_SUSTAIN:
				if (!eg->gate || !bank->sustain) eg->status = USYNTH_EG_RELEASE;
				break;
			
			case USYNTH_EG_RELEASE:
				if (eg->gate)
				{
					eg->value = 0;
					eg->status = USYNTH_EG_ATTACK;
				}
				else
				{
					if (eg->value < bank->release)
					{
						eg->value = 0;
						eg->status = USYNTH_EG_IDLE;
					}
					else
					{
						eg->value -= bank->release;
					}
				}
				break;
		}
	
		eg->output = (eg->value * (uint32_t) bank->sustain) >> 16;
	}
}	

#endif