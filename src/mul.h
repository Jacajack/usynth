#ifndef MUL_H
#define MUL_H

/**
	\file Inline assmebly for integer multiplication

	Based on Norbert Pozar's https://github.com/rekka/avrmultiplication
*/

#define MUL_U16_U16_16H(intRes, intIn1, intIn2) \
asm volatile ( \
"clr r26 \n\t" \
"mul %A1, %A2 \n\t" \
"mov r27, r1 \n\t" \
"mul %B1, %B2 \n\t" \
"movw %A0, r0 \n\t" \
"mul %B2, %A1 \n\t" \
"add r27, r0 \n\t" \
"adc %A0, r1 \n\t" \
"adc %B0, r26 \n\t" \
"mul %B1, %A2 \n\t" \
"add r27, r0 \n\t" \
"adc %A0, r1 \n\t" \
"adc %B0, r26 \n\t" \
"clr r1 \n\t" \
: \
"=&r" (intRes) \
: \
"a" (intIn1), \
"a" (intIn2) \
: \
"r26" , "r27" \
) 

#define MUL_S16_U16_16H(intRes, intIn1, intIn2) \
asm volatile ( \
"clr r26 \n\t" \
"mul %A1, %A2 \n\t" \
"mov r27, r1 \n\t" \
"mulsu %B1, %B2 \n\t" \
"movw %A0, r0 \n\t" \
"mul %B2, %A1 \n\t" \
"add r27, r0 \n\t" \
"adc %A0, r1 \n\t" \
"adc %B0, r26 \n\t" \
"mulsu %B1, %A2 \n\t" \
"sbc %B0, r26 \n\t" \
"add r27, r0 \n\t" \
"adc %A0, r1 \n\t" \
"adc %B0, r26 \n\t" \
"clr r1 \n\t" \
: \
"=&r" (intRes) \
: \
"a" (intIn1), \
"a" (intIn2) \
: \
"r26", "r27" \
)

#define MUL_U16_U8_16H(intRes, int16In, int8In) \
asm volatile ( \
"clr r26 \n\t"\
"mul %B1, %A2 \n\t"\
"movw %A0, r0 \n\t"\
"mul %A1, %A2 \n\t"\
"add %A0, r1 \n\t"\
"adc %B0, r26 \n\t"\
"clr r1 \n\t"\
: \
"=&r" (intRes) \
: \
"a" (int16In), \
"a" (int8In) \
:\
"r26"\
)



#endif