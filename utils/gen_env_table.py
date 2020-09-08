#!/usr/bin/python3
import math

for k in range(0, 128):
	x = int(65536.0 / ((k*2)+1) - 256)
	print("\t{},\t// {}".format(x, k));
