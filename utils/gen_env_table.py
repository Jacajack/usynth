#!/usr/bin/python3
import math

for k in range(0, 256):
	x = int(-1 + math.exp(-(k - 300) / 30))
	print("\t{},\t// {}".format(x, k));
