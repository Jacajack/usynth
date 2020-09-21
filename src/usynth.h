#ifndef USYNTH_H
#define USYNTH_H

#include "ppg/ppg_osc.h"
#include "eg.h"
#include "lfo.h"

// LED IO defs
#define LED_1_PIN 2
#define LED_2_PIN 3
#define LED_3_PIN 4
#define LED_RED_PIN LED_1_PIN
#define LED_YLW_PIN LED_2_PIN
#define LED_GRN_PIN LED_3_PIN

// DAC IO defs
#define LDAC_PIN  1
#define CS_PIN    2
#define MOSI_PIN  3
#define SCK_PIN   5

// DAC config bits
#define MCP4921_DAC_AB_BIT   (1 << 15)
#define MCP4921_VREF_BUF_BIT (1 << 14)
#define MCP4921_GAIN_BIT     (1 << 13)
#define MCP4921_SHDN_BIT     (1 << 12)

/**
	A single synthesizer voice
*/
typedef struct usynth_voice
{
	ppg_osc osc;
	usynth_eg amp_eg;
	usynth_eg mod_eg;
	usynth_lfo lfo;
	int8_t base_wave;
	int8_t eg_mod_int;
	int8_t lfo_mod_int;
	int8_t eg_pitch_int;
	int8_t lfo_pitch_int;
	uint8_t wavetable_number;
} usynth_voice;


#endif