#include "midi_program.h"
#include <avr/pgmspace.h>
#include "midi.h"
#include "midi_cc.h"

#define MIDI_PARAM_PROGRAM_BEGIN     0xff
#define MIDI_PARAM_PROGRAM_TABLE_END 0xfe
#define MIDI_PROGRAM_BEGIN(id) {MIDI_PARAM_PROGRAM_BEGIN, (id)}
#define MIDI_PROGRAM_TABLE_END() {MIDI_PARAM_PROGRAM_TABLE_END, 0}
#define MIDI_BOTH (1 << 7)

// Table forward declaration
static const midi_program_data midi_program_table[] PROGMEM;

/**
	Loads MIDI preset
*/
void midi_program_load(midi_status *midi, uint8_t id)
{
	// Load defaults before loading anything
	if (id != 0xff)
		midi_program_load(midi, 0xff);

	const midi_program_data *ptr;
	uint8_t param = 0;
	uint8_t value = 0;
	uint8_t found = 0;

	for (ptr = midi_program_table; param != MIDI_PARAM_PROGRAM_TABLE_END; ptr++)
	{
		param = pgm_read_byte(&ptr->param);
		value = pgm_read_byte(&ptr->value);
		
		// If on desired program label, start writing
		// If already writing, break
		if (param == MIDI_PARAM_PROGRAM_BEGIN)
		{
			if (found)
				break;
			else if (value == id)
				found = 1;
		}
		else if (found)
		{
			// If the highest bit is set, the next CC is set as well
			// Handy when setting CC pairs
			midi->control[param & 0x7f] = value & 0x7f;
			if (param & MIDI_BOTH)
				midi->control[(param & 0x7f) + 1] = value & 0x7f;
		}
	}

	// On succesful load
	if (found)
		midi->program = id;
}

/**
	Default MIDI programs

	\warning These MIDI programs cannot set CC 127 and 126,
	because those are reserved for MIDI_PROGRAM_BEGIN
	and MIDI_PROGRAM_TABLE_END
*/
static const midi_program_data midi_program_table[] PROGMEM =
{
	// The defaults loaded before loading any program
	MIDI_PROGRAM_BEGIN(0xff),
	{MIDI_BOTH | MIDI_OSC_WAVETABLE(0),    0  },
	{MIDI_BOTH | MIDI_OSC_BASE_WAVE(0),    64 },
	{MIDI_BOTH | MIDI_OSC_DETUNE(0),       64 },
	{MIDI_BOTH | MIDI_OSC_PITCH(0),        64 },
	{MIDI_BOTH | MIDI_OSC_VOLUME(0),       127},

	{MIDI_BOTH | MIDI_AMP_A(0),            0  },
	{MIDI_BOTH | MIDI_AMP_S(0),            127},
	{MIDI_BOTH | MIDI_AMP_R(0),            0  },
	{MIDI_BOTH | MIDI_AMP_ASR(0),          1  },

	{MIDI_BOTH | MIDI_EG_A(0),             0  },
	{MIDI_BOTH | MIDI_EG_S(0),             127},
	{MIDI_BOTH | MIDI_EG_R(0),             0  },
	{MIDI_BOTH | MIDI_EG_ASR(0),           1  },
	{MIDI_BOTH | MIDI_EG_MOD_INT(0),       64 },
	{MIDI_BOTH | MIDI_EG_PITCH_INT(0),     64 },

	{MIDI_BOTH | MIDI_LFO_RATE(0),         64 },
	{MIDI_BOTH | MIDI_LFO_WAVE(0),         0  },
	{MIDI_BOTH | MIDI_LFO_FADE(0),         0  },
	{MIDI_BOTH | MIDI_LFO_MOD_INT(0),      64 },
	{MIDI_BOTH | MIDI_LFO_PITCH_INT(0),    64 },

	{MIDI_LFO_SYNC,  0 },
	{MIDI_LFO_RESET, 0 },
	{MIDI_POLY,      1 },
	{MIDI_CUTOFF,    64},


	// Program 0 - some kind of marimba
	MIDI_PROGRAM_BEGIN(0),
	// {MIDI_WAVETABLE,    2},
	// {MIDI_BASE_WAVE,    0},
	
	// {MIDI_AMP_ATTACK,   0},
	// {MIDI_AMP_SUSTAIN,  127},
	// {MIDI_AMP_RELEASE,  91},
	// {MIDI_AMP_ASR,      1},
	
	// {MIDI_MOD_ATTACK,   0},
	// {MIDI_MOD_SUSTAIN,  90},
	// {MIDI_MOD_RELEASE,  76},
	// {MIDI_MOD_ASR,      0},
	// {MIDI_MOD_EG_INT,  127},


	// Program 1 - Morpher
	MIDI_PROGRAM_BEGIN(1),
	// {MIDI_WAVETABLE,    9},
	// {MIDI_BASE_WAVE,    102},
	
	// {MIDI_AMP_ATTACK,   15},
	// {MIDI_AMP_SUSTAIN,  127},
	// {MIDI_AMP_RELEASE,  108},
	// {MIDI_AMP_ASR,      1},
	
	// {MIDI_MOD_ATTACK,   0},
	// {MIDI_MOD_SUSTAIN,  127},
	// {MIDI_MOD_RELEASE,  114},
	// {MIDI_MOD_ASR,      0},
	// {MIDI_MOD_EG_INT,   0},

	// {MIDI_CUTOFF,       104},


	MIDI_PROGRAM_TABLE_END(),
};

