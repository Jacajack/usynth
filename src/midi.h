#ifndef MIDI_H
#define MIDI_H

#include <inttypes.h>

#ifndef MIDI_MAX_VOICES
#error MIDI_MAX_VOICES is not defined
#endif

#if !defined(MIDI_POLY_REPLACE_OLDEST) && !defined(MIDI_POLY_REPLACE_NEWEST)
#define MIDI_POLY_REPLACE_OLDEST
#endif

#ifdef MIDI_POLY_REPLACE_OLDEST 
#define MIDI_AGE_COMP(x, y) ((x) > (y))
#define MIDI_WORST_AGE (-1)
#endif

#ifdef MIDI_POLY_REPLACE_NEWEST
#define MIDI_AGE_COMP(x, y) ((x) < (y))
#define MIDI_WORST_AGE 127
#endif

// Gate states
#define MIDI_GATE_ON_BIT   (1 << 0)
#define MIDI_GATE_TRIG_BIT (1 << 1)

// Program data macros
#define MIDI_PARAM_PROGRAM_BEGIN     0xff
#define MIDI_PARAM_PROGRAM_TABLE_END 0xfe
#define MIDI_PROGRAM_BEGIN(id) {MIDI_PARAM_PROGRAM_BEGIN, (id)}
#define MIDI_PROGRAM_TABLE_END() {MIDI_PARAM_PROGRAM_TABLE_END, 0}
#define MIDI_BOTH (1 << 7)

typedef struct midi_program_data
{
	uint8_t param;
	uint8_t value;
} midi_program_data;

typedef struct midi_voice
{
	uint8_t note;
	uint8_t velocity;
	uint8_t gate;
	int8_t age;
} midi_voice;

typedef struct midi_status
{
	// MIDI controllers
	uint8_t control[128];

	// Per voice controls
	midi_voice voices[MIDI_MAX_VOICES];
	uint8_t voice_count;

	// Internal state of the interpreter
	uint8_t dlim;
	uint8_t dcnt;
	uint8_t status;
	uint8_t channel;
	uint8_t dbuf[4];

	// Basic MIDI controls
	uint8_t program;
	uint16_t pitchbend;
} midi_status;


extern void midi_init(midi_status *midi, uint8_t voice_count);
extern void midi_program_load(midi_status *midi, uint8_t id);


/**
	Handles Note ON events.

	Finds an empty voice slot and uses it or overwrites the oldest
	active slot
*/
static inline void midi_note_on(midi_status *midi, uint8_t note, uint8_t velocity) __attribute__((always_inline));
static inline void midi_note_on(midi_status *midi, uint8_t note, uint8_t velocity)
{
	uint8_t best_empty_slot = MIDI_MAX_VOICES;
	uint8_t best_active_slot = 0;
	int8_t best_empty = MIDI_WORST_AGE;
	int8_t best_active = MIDI_WORST_AGE;

	for (uint8_t i = 0; i < midi->voice_count; i++)
	{
		int8_t age = midi->voices[i].age;

		if (midi->voices[i].gate)
		{
			if (MIDI_AGE_COMP(age, best_active))
			{
				best_active_slot = i;
				best_active = midi->voices[i].age;
			}
		}
		else 
		{
			if (MIDI_AGE_COMP(age, best_empty))
			{
				best_empty_slot = i;
				best_empty = age;
			}
		}
	}

	// Increment age of all voices
	for (uint8_t i = 0; i < midi->voice_count; i++)
		midi->voices[i].age++;

	// Always prefer empty slot over reuse
	uint8_t slot = best_empty_slot == MIDI_MAX_VOICES ? best_active_slot : best_empty_slot;
	midi->voices[slot].gate = MIDI_GATE_ON_BIT | MIDI_GATE_TRIG_BIT;
	midi->voices[slot].note = note;
	midi->voices[slot].velocity = velocity;
	midi->voices[slot].age = 0;	
}

/**
	Handles Note OFF events
*/
static inline void midi_note_off(midi_status *midi, uint8_t note) __attribute__((always_inline));
static inline void midi_note_off(midi_status *midi, uint8_t note)
{
	for (uint8_t i = 0; i < MIDI_MAX_VOICES; i++)
		if (midi->voices[i].note == note)
			midi->voices[i].gate = 0;
}

static inline void midi_process_byte(midi_status *midi, uint8_t byte, uint8_t channel)
{
	uint8_t dlim = midi->dlim;
	uint8_t dcnt = midi->dcnt;
	uint8_t status = midi->status;

	if (byte & (1 << 7)) // Handle status bytes
	{
		// Extract information from status byte
		status = byte & 0x70;
		midi->channel = byte & 0x0f;

		const static uint8_t dlim_table[16] __attribute__((aligned(16))) = 
		{
			[0] = 2, // Note off
			[1] = 2, // Note on
			[3] = 2, // CC change
			[4] = 1, // Program change
			[6] = 2, // Pitch change
		};

		dcnt = 0;
		dlim = dlim_table[status >> 4];
	}
	else if (midi->channel == channel) // Handle data bytes
	{
		// Data byte
		midi->dbuf[dcnt] = byte;

		// Interpret command
		if (++dcnt == dlim)
		{
			switch (status)
			{
				// Note on
				case 0x10:
					midi_note_on(midi, midi->dbuf[0], midi->dbuf[1]);
					break;

				// Note off
				case 0x00:
					midi_note_off(midi, midi->dbuf[0]);
					break;

				// Controller change
				case 0x30:
					midi->control[midi->dbuf[0]] = midi->dbuf[1];
					break;

				// Program change
				case 0x40:
					midi_program_load(midi, midi->dbuf[0]);
					break;

				// Pitch
				case 0x60:
					midi->pitchbend = midi->dbuf[0] | (midi->dbuf[1] << 7);
					break;
			}

			dcnt = 0;
		}

	}

	// Write status back to the struct
	midi->dlim = dlim;
	midi->dcnt = dcnt;
	midi->status = status;
}

static inline void midi_clear_trig_bits(midi_status *midi)
{
	for (uint8_t i = 0; i < MIDI_MAX_VOICES; i++)
		midi->voices[i].gate &= ~MIDI_GATE_TRIG_BIT;
}

#endif
