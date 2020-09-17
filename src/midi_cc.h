#ifndef MIDI_CC_H
#define MIDI_CC_H

// Defines two separate controllers - x and x + 1
#define MIDI_CC_PAIR(v, x) ((v) ? (x + 1) : (x))

// Standard MIDI controls
#define MIDI_MODULATION         1

// Oscillator controls
#define MIDI_OSC_WAVETABLE(n)   MIDI_CC_PAIR(n, 20)
#define MIDI_OSC_BASE_WAVE(n)   MIDI_CC_PAIR(n, 22)
#define MIDI_OSC_DETUNE(n)      MIDI_CC_PAIR(n, 24)
#define MIDI_OSC_VOLUME(n)      MIDI_CC_PAIR(n, 26)
#define MIDI_OSC_PITCH(n)       MIDI_CC_PAIR(n, 28)

// Amplifier envelope controls
#define MIDI_AMP_A(n)           MIDI_CC_PAIR(n, 30)
#define MIDI_AMP_S(n)           MIDI_CC_PAIR(n, 32)
#define MIDI_AMP_R(n)           MIDI_CC_PAIR(n, 34)
#define MIDI_AMP_ASR(n)         MIDI_CC_PAIR(n, 36)

// Modulation envelope controls
#define MIDI_EG_A(n)            MIDI_CC_PAIR(n, 40)
#define MIDI_EG_S(n)            MIDI_CC_PAIR(n, 42)
#define MIDI_EG_R(n)            MIDI_CC_PAIR(n, 44)
#define MIDI_EG_ASR(n)          MIDI_CC_PAIR(n, 46)
#define MIDI_EG_MOD_INT(n)      MIDI_CC_PAIR(n, 48)
#define MIDI_EG_PITCH_INT(n)    MIDI_CC_PAIR(n, 50)

// LFO
#define MIDI_LFO_RATE(n)        MIDI_CC_PAIR(n, 60)
#define MIDI_LFO_WAVE(n)        MIDI_CC_PAIR(n, 62)
#define MIDI_LFO_FADE(n)        MIDI_CC_PAIR(n, 64)
#define MIDI_LFO_MOD_INT(n)     MIDI_CC_PAIR(n, 66)
#define MIDI_LFO_PITCH_INT(n)   MIDI_CC_PAIR(n, 68)

// Common controls
#define MIDI_LFO_SYNC           100
#define MIDI_LFO_RESET          101
#define MIDI_POLY               102
#define MIDI_CUTOFF             103

// Debug
#define MIDI_DEBUG_CHANNEL      111

#endif