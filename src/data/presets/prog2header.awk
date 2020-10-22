#!/usr/bin/awk -f

BEGIN {
	defaults[20] = 0;
	defaults[21] = 0;
	defaults[22] = 64;
	defaults[23] = 64;
	defaults[24] = 64;
	defaults[25] = 64;
	defaults[28] = 64;
	defaults[29] = 64;
	
	defaults[30] = 0;
	defaults[31] = 0;
	defaults[32] = 127;
	defaults[33] = 127;
	defaults[34] = 0;
	defaults[35] = 0;
	defaults[36] = 1;
	defaults[37] = 1;
	
	defaults[40] = 0;
	defaults[41] = 0;
	defaults[42] = 127;
	defaults[43] = 127;
	defaults[44] = 0;
	defaults[45] = 0;
	defaults[46] = 1;
	defaults[47] = 1;
	defaults[48] = 64;
	defaults[49] = 64;
	defaults[50] = 64;
	defaults[51] = 64;
	
	defaults[60] = 64;
	defaults[61] = 64;
	defaults[62] = 0;
	defaults[63] = 0;
	defaults[64] = 0;
	defaults[65] = 0;
	defaults[66] = 64;
	defaults[67] = 64;
	defaults[68] = 64;
	defaults[69] = 64;
	
	defaults[100] = 0;
	defaults[101] = 0;
	defaults[102] = 1;
	defaults[103] = 64;
}

{
	ctls[$1] = $2;
}

END {
	for (n in ctls)
	{
		# Both controls have the same value
		if (n % 2 == 0 && ctls[n] == ctls[n + 1])
		{
			if (ctls[n] != defaults[n])
			{
				printf("{%d, %d},\n", n + 127, ctls[n]);
			}
		}
		else if (ctls[n] != defaults[n] && (n % 2 == 0 || (n % 2 == 1 && ctls[n] != ctls[n - 1])))
		{
			printf("{%d, %d},\n", n, ctls[n]);
		}
	}
}
