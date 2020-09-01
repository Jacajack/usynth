#!/usr/bin/python3
import math

for k in range(0, 127):
	f = 440.0 * math.exp((k - 69) * math.log(math.pow(2, 1 / 12)));
	print("\t65536 * {:5.3f} / F_SAMPLE,".format(f));
