#include <avr/io.h>
#include <avr/interrupt.h>
#include <inttypes.h>

#include "ppg/ppg.h"

#ifndef F_CPU
#error F_CPU is not defined!
#endif

#ifndef F_SAMPLE
#error F_SAMPLE is not defined!
#endif

#ifndef MIDI_BAUD
#warning MIDI_BAUD is not defined! Assuming default 31250
#define MIDI_BAUD 31250
#endif



#define LED_1_PIN 2
#define LED_2_PIN 3
#define LED_3_PIN 4

#define LDAC_PIN  1
#define CS_PIN    2
#define MOSI_PIN  3
#define SCK_PIN   5

static void spi_txb(uint8_t b)
{
	SPDR = b;
	while (!(SPSR & (1 << SPIF)));
}

#define MCP4921_DAC_AB_BIT   (1 << 15)
#define MCP4921_VREF_BUF_BIT (1 << 14)
#define MCP4921_GAIN_BIT     (1 << 13)
#define MCP4921_SHDN_BIT     (1 << 12)

/**
	Main interrupt - sends a new sample to the DAC
*/
volatile uint16_t dac_data = 0;
volatile uint8_t dac_sent = 0;
ISR(TIMER1_COMPB_vect)
{
	// CS is automatically set low
	
	const uint16_t mcp4921_conf = MCP4921_SHDN_BIT | MCP4921_GAIN_BIT | MCP4921_VREF_BUF_BIT;
	uint16_t data = mcp4921_conf | (dac_data >> 4);
	spi_txb(data >> 8);
	spi_txb(data);
	
	// Force compare event to set CS high
	TCCR1A = (1 << COM1B0) | (1 << COM1B1);
	TCCR1C |= (1 << FOC1B);
	TCCR1A = (1 << COM1B1);
	
	// Set sent flag, so the main loop can work again
	dac_sent = 1;
}

int main(void)
{
	// LEDs
	DDRD |= (1 << LED_1_PIN) | (1 << LED_2_PIN) | (1 << LED_3_PIN);
	
	// 10 MHz SPI Master, DAC IO
	PORTB |= (1 << LDAC_PIN) | (1 << CS_PIN);
	DDRB |= (1 << LDAC_PIN) | (1 << CS_PIN) | (1 << MOSI_PIN) | (1 << SCK_PIN);
	SPCR = (1 << SPE) | (1 << MSTR);
	SPSR = (1 << SPI2X);
	
	// LDAC tied low, CS is used as word clock
	PORTB &= ~(1 << LDAC_PIN);
	
	/*
		TIMER 1 - CTC mode, F_CPU

		- Clear OC1B (CS) on compare
		- COMP1B ISR active
	*/
	TCCR1B = (1 << WGM12) | (1 << CS10);
	TIMSK1 = (1 << OCIE1B);
 	OCR1A = F_CPU / F_SAMPLE - 1;
 	OCR1B = 0;
	
	// Set CS high and enable 'clear on compare'
	TCCR1A = (1 << COM1B0) | (1 << COM1B1);
	TCCR1C |= (1 << FOC1B);
	TCCR1A = (1 << COM1B1);
	
	
	/*
		USART0 - MIDI, 8 bit data, 1 bit stop, no parity
	*/
	UBRR0 = F_CPU / 16 / MIDI_BAUD - 1;
	UCSR0B = (1 << RXEN0) | (1 << TXEN0);
	UCSR0C = (1 << UCSZ00) | (1<< UCSZ01);
	
	wavetable_entry wt[DEFAULT_WAVETABLE_SIZE];
	load_wavetable_n(wt, DEFAULT_WAVETABLE_SIZE, ppg_wavetable, 18);

	sei();

	// The main loop
	while (1)
	{
		static uint16_t t = 0;
		static int8_t dt = 0;
		static uint16_t phase = 0;
		static uint16_t phase_step = 65536 * 55 / F_SAMPLE;
		
		uint8_t n = t >> 10;
		n = n > 60 ? 60 : n;
		dac_data = get_wavetable_sample(wt + n, phase);
		phase += phase_step;
		
		if (t == 0) dt = 1;
		else if (t == 65535) dt = -1;
		t += dt;;
		
		// Wait for sent flag and clear it
		while (!dac_sent);
		dac_sent = 0;
	}
}
