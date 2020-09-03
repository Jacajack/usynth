#ifndef EG_H
#define EG_H

#include "config.h"
#include "mul.h"

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
	uint16_t release;
	uint8_t sustain;
	uint8_t sustain_enabled;
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
					uint16_t tmp = eg->value + bank->attack;
					if (tmp < eg->value)
					{
						eg->value = UINT16_MAX;
						eg->status = USYNTH_EG_SUSTAIN;
					}
					else
					{
						eg->value = tmp;
					}
				}
				break;
				
			case USYNTH_EG_SUSTAIN:
				if (!eg->gate || !bank->sustain_enabled) eg->status = USYNTH_EG_RELEASE;
				break;
			
			case USYNTH_EG_RELEASE:
				if (eg->gate && bank->sustain_enabled)
				{
					eg->value = 0;
					eg->status = USYNTH_EG_ATTACK;
				}
				else
				{
					uint16_t tmp = eg->value - bank->release;
					if (tmp > eg->value)
					{
						eg->value = 0;
						eg->status = USYNTH_EG_IDLE;
					}
					else
					{
						eg->value = tmp;
					}
				}
				break;
		}
	
		MUL_U16_U8_16H(eg->output, eg->value, bank->sustain);
	}
}	

#endif