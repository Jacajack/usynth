#define F_CPU 20000000UL

#include <util/delay.h>
#include <avr/io.h>

int main(void)
{
	DDRD |= (1 << 2) | (1 << 3) | (1 << 4);
	
	unsigned char b = (1 << 2);
	while (1)
	{
		PORTD = b;
		b <<= 1;
		if (b == (1 << 5))
			b = (1 << 2);
		
		_delay_ms(100);
	}
}
