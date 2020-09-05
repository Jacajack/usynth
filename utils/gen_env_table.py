#!/usr/bin/python3
import math

for k in range(0, 256):
	x = int(65536.0 / (k+1) - 255)
	print("\t{},\t// {}".format(x, k));
