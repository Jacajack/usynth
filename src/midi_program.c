#include "midi_program.h"
#include <avr/pgmspace.h>
#include "midi.h"
#include "midi_cc.h"

/**
	Default MIDI programs

	\warning These MIDI programs cannot set CC 127 and 126,
	because those are reserved for MIDI_PROGRAM_BEGIN
	and MIDI_PROGRAM_TABLE_END
*/
const midi_program_data midi_program_table[] PROGMEM =
{
	// The defaults loaded before loading any program
	MIDI_PROGRAM_BEGIN(0xff),
	{MIDI_BOTH | MIDI_OSC_WAVETABLE(0),    0  },
	{MIDI_BOTH | MIDI_OSC_BASE_WAVE(0),    64 },
	{MIDI_BOTH | MIDI_OSC_DETUNE(0),       64 },
	{MIDI_BOTH | MIDI_OSC_PITCH(0),        64 },

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

#include "data/presets/generated.h"

	MIDI_PROGRAM_TABLE_END(),
};

