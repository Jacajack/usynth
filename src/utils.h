#ifndef UTILS_H
#define UTILS_H

#define SAFE_ADD_INT8(res, x) if ((x) > 0) {if (127 - (x) < (res)) (res) = 127; else (res) += (x);} else {if (-128 - (x) > (res)) (res) = -128; else (res) += (x);}
#define MIDI_CONTROL_TO_S8(x) (((int8_t)(x) - 64) << 1)
#define MIDI_CONTROL_TO_U8(x) ((x) << 1)

#endif