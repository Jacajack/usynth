#ifndef USYNTH_H
#define USYNTH_H

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

#endif