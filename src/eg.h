#ifndef EG_H
#define EG_H

#include "config.h"
#include "mul.h"

#define USYNTH_EG_IDLE    0
#define USYNTH_EG_ATTACK  1
#define USYNTH_EG_SUSTAIN 2
#define USYNTH_EG_RELEASE 3

typedef struct usynth_eg
{
	uint16_t attack;
	uint16_t release;
	uint8_t sustain;
	uint8_t sustain_enabled;

	uint8_t gate;
	uint8_t status;
	uint16_t value;
	uint16_t output;
} usynth_eg;

static inline void usynth_eg_update(usynth_eg *eg)
{
	switch (eg->status)
	{
		case USYNTH_EG_IDLE:
			if (eg->gate) eg->status = USYNTH_EG_ATTACK;
			break;
				
		case USYNTH_EG_ATTACK:
			if (!eg->gate) eg->status = USYNTH_EG_RELEASE;
			else
			{
				uint16_t tmp = eg->value + eg->attack;
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
			if (!eg->gate || !eg->sustain_enabled) eg->status = USYNTH_EG_RELEASE;
			break;
		
		case USYNTH_EG_RELEASE:
			{
				uint16_t tmp = eg->value - eg->release;
				if (tmp > eg->value)
				{
					eg->value = 0;
				}
				else
				{
					eg->value = tmp;
				}
			}
			break;
	}
	
	MUL_U16_U8_16H(eg->output, eg->value, eg->sustain);
}	

#endif