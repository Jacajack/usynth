#ifndef MIDI_H
#define MIDI_H
#include <inttypes.h>

#define MIDI_VOICES 2

typedef struct midi_voice
{
	uint8_t note;
	uint8_t velocity;
	uint8_t gate;
	uint8_t age;
} midi_voice;

typedef struct midi_status
{
	// Internal state of the interpreter
	uint8_t dlim;
	uint8_t dcnt;
	uint8_t status;
	uint8_t channel;
	uint8_t dbuf[4];

	// Per voice controls
	midi_voice voices[MIDI_VOICES];

	// Basic MIDI controls
	uint8_t program;
	uint16_t pitchbend;
	uint8_t reset;

	// The controller represented by a union of an array and aliases
	union
	{
		struct __attribute__((packed))
		{
			// 00 - 0F
			uint8_t bank;
			uint8_t modulation;
			uint8_t breath;
			uint8_t cc3;
			uint8_t foot;
			uint8_t portamento_time;
			uint8_t dmsb;
			uint8_t volume;
			uint8_t balance;
			uint8_t cc9;
			uint8_t pan;
			uint8_t expression;
			uint8_t fxctl1;
			uint8_t fxctl2;
			uint8_t cc14;
			uint8_t cc15;

			// 0F - 1F
			uint8_t korg_attack;
			uint8_t korg_decay;
			uint8_t korg_sustain;
			uint8_t korg_release;
			uint8_t cc20;
			uint8_t cc21;
			uint8_t cc22;
			uint8_t cc23;
			uint8_t cc24;
			uint8_t cc25;
			uint8_t cc26;
			uint8_t cc27;
			uint8_t cc28;
			uint8_t cc29;
			uint8_t cc30;
			uint8_t cc31;

			uint8_t c0lsb;
			uint8_t c1lsb;
			uint8_t c2lsb;
			uint8_t c3lsb;
			uint8_t c4lsb;
			uint8_t c5lsb;
			uint8_t c6lsb;
			uint8_t c7lsb;
			uint8_t c8lsb;
			uint8_t c9lsb;
			uint8_t c10lsb;
			uint8_t c11lsb;
			uint8_t c12lsb;
			uint8_t c13lsb;
			uint8_t c14lsb;
			uint8_t c15lsb;
			uint8_t c16lsb;
			uint8_t c17lsb;
			uint8_t c18lsb;
			uint8_t c19lsb;
			uint8_t c20lsb;
			uint8_t c21lsb;
			uint8_t c22lsb;
			uint8_t c23lsb;
			uint8_t c24lsb;
			uint8_t c25lsb;
			uint8_t c26lsb;
			uint8_t c27lsb;
			uint8_t c28lsb;
			uint8_t c29lsb;
			uint8_t c30lsb;
			uint8_t c31lsb;
			uint8_t damper;
			uint8_t portmamento_enabled;
			uint8_t sostenuto;
			uint8_t softpedal;
			uint8_t legato;
			uint8_t hold2;
			uint8_t sndctl1;
			uint8_t sndctl2; //Resonance control
			uint8_t sndctl3; //Release time
			uint8_t sndctl4; //Attack time
			uint8_t sndctl5; //Cutoff control
			uint8_t sndctl6;
			uint8_t sndctl7;
			uint8_t sndctl8;
			uint8_t sndctl9;
			uint8_t sndctl10;
			uint8_t gpctl80;
			uint8_t gpctl81;
			uint8_t gpctl82;
			uint8_t gpctl83;
			uint8_t portamento_control;
			uint8_t cc85;
			uint8_t cc86;
			uint8_t cc87;
			uint8_t cc88;
			uint8_t cc89;
			uint8_t cc90;
			uint8_t fxd1; // Reverb amount
			uint8_t fxd2; // Tremolo amount
			uint8_t fxd3; // Chorus amount
			uint8_t fxd4; // Detune amount
			uint8_t fxd5; // Phaser amount
			uint8_t dinc;
			uint8_t ddec;
			uint8_t nonreglsb;
			uint8_t nonregmsb;
			uint8_t reglsb;
			uint8_t regmsb;
			uint8_t cc102;
			uint8_t cc103;
			uint8_t cc104;
			uint8_t cc105;
			uint8_t cc106;
			uint8_t cc107;
			uint8_t cc109;
			uint8_t cc110;
			uint8_t cc111;
			uint8_t cc112;
			uint8_t cc113;
			uint8_t cc114;
			uint8_t cc115;
			uint8_t cc116;
			uint8_t cc117;
			uint8_t cc118;
			uint8_t cc119;
			uint8_t allsoundsoff;
			uint8_t ctlrst;
			uint8_t local;
			uint8_t allnotesoff;
			uint8_t omnion;
			uint8_t omioff;
			uint8_t mono;
			uint8_t poly;
		};

		uint8_t raw[128];
	} controllers;
} midi_status;

extern void midiproc(struct midi_status *midi, uint8_t byte, uint8_t channel);

#endif
