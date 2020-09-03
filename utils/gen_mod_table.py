#!/usr/bin/python3
import math
from itertools import chain

for k in chain(range(0, 128), range(-128, 0)):
	x = (k + 128) >> 2
	if x > 60:
		x = 60
	print("\t{},\t// {}".format(x, k));
