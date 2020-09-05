#!/usr/bin/python3
import math

F_SAMPLE = 28000

print("""#include "notes_table.h"
#include <inttypes.h>
#include <avr/pgmspace.h>

/**
	Compressed note frequency table
	Generated for F_SAMPLE = {}
*/
const uint8_t notes_table[] PROGMEM =
{{""".format(F_SAMPLE))

f_last = 0
v_last = 0
for k in range(0, 128 * 32):
	f = 440.0 * math.exp(((k / 32) - 69) * math.log(math.pow(2, 1 / 12)));	
	v = 65536 * f / F_SAMPLE
	if (k % 2 == 0):
		print("\t{}, {},".format(int(v) & 255, int(v) >> 8))
	else:
		print("\t{},".format(int(v - v_last)))
	v_last = v;
	f_last = f;
	# print("\t65536 * {:5.3f} / F_SAMPLE,".format(f));
	# print("delta_f = {}".format(f - f_last))	

print("""};
""")