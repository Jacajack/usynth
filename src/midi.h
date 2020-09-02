#ifndef MIDI_H
#define MIDI_H
#include <inttypes.h>

#include "config.h"

#ifndef MIDI_VOICES
#error MIDI_VOICES is not defined
#endif

typedef struct midi_voice
{
	uint8_t note;
	uint8_t velocity;
	uint8_t gate;
	int8_t age;
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

	// MIDI controllers
	uint8_t control[128];
} midi_status;

extern void midi_init(midi_status *midi);
extern void midi_process_byte(midi_status *midi, uint8_t byte, uint8_t channel);

#endif
