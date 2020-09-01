#include <avr/io.h>
#include <avr/interrupt.h>
#include <inttypes.h>

#include "ppg/ppg.h"
#include "midi.h"
#include "notes.h"

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

#define USYNTH_EG_IDLE    0
#define USYNTH_EG_ATTACK  1
#define USYNTH_EG_SUSTAIN 2
#define USYNTH_EG_RELEASE 3

typedef struct usynth_eg
{
	uint16_t attack;
	uint16_t sustain;
	uint16_t release;
	
	uint16_t value;
	uint16_t output;
	
	uint8_t status;
} usynth_eg;


static inline uint16_t usynth_eg_update(usynth_eg *eg, uint8_t gate)
{
	switch (eg->status)
	{
		case USYNTH_EG_IDLE:
			if (gate) eg->status = USYNTH_EG_ATTACK;
			break;
				
		case USYNTH_EG_ATTACK:
			if (!gate) eg->status = USYNTH_EG_RELEASE;
			else
			{
				if (UINT16_MAX - eg->attack < eg->value)
				{
					eg->value = UINT16_MAX;
					eg->status = USYNTH_EG_SUSTAIN;
				}
				else
				{
					eg->value += eg->attack;
				}
			}
			break;
			
		case USYNTH_EG_SUSTAIN:
			if (!gate) eg->status = USYNTH_EG_RELEASE;
			break;
		
		case USYNTH_EG_RELEASE:
			if (gate)
			{
				eg->value = 0;
				eg->status = USYNTH_EG_ATTACK;
			}
			else
			{
				if (eg->value < eg->release)
				{
					eg->value = 0;
					eg->status = USYNTH_EG_IDLE;
				}
				else
				{
					eg->value -= eg->release;
				}
			}
			break;
	}
	
	eg->output = (eg->value * (uint32_t) eg->sustain) >> 16;
	return eg->output;
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
	load_wavetable_n(wt, DEFAULT_WAVETABLE_SIZE, ppg_wavetable, 1);

	midistatus midi;
	
	usynth_eg eg = {
		.status = USYNTH_EG_IDLE,
		.attack = 65535,
		.release = 40,
		.sustain = 65535,
		.value = 0
	};
	
	usynth_eg eg2 = eg;
	
	sei();
	
	// The main loop
	uint8_t slow_cnt = 0;
	uint8_t sw = 0, last_gate = 0;
	while (1)
	{
		// Check incoming USART data and process it
		if (UCSR0A & (1 << RXC0))
			midiproc(&midi, UDR0, 0);
		
		
		static uint16_t phase_step = 0, phase_step2 = 0;
		if (!last_gate && midi.noteon)
		{
			if (sw)
			{
				phase_step = pgm_read_word(midi_notes + midi.note);
			}
			else
			{
				phase_step2 = pgm_read_word(midi_notes + midi.note);
			}
			
			sw = !sw;
		}
		last_gate = midi.noteon;
		
		// Slow calculations done each 16 samples
		if (slow_cnt++ == 16)
		{
			slow_cnt = 0;
			usynth_eg_update(&eg, midi.noteon && sw);
			usynth_eg_update(&eg2, midi.noteon && !sw);
		}
		
		if (midi.noteon)
			PORTD |= (1 << LED_1_PIN);
		else
			PORTD &= ~(1 << LED_1_PIN);
		
		static uint16_t t = 0;
		static int8_t dt = 0;
		static uint16_t phase = 0, phase2 = 0;
// 		uint16_t phase_step = pgm_read_word(midi_notes + midi.note);
		
		uint8_t n = 64 - (eg.output >> 10);
		n = n > 60 ? 60 : n;
		
		uint8_t n2 = 64 - (eg2.output >> 10);
		n2 = n2 > 60 ? 60 : n2;
		
		dac_data = ((get_wavetable_sample(wt + n, phase) * (uint32_t)eg.output) >> 17)
			+ ((get_wavetable_sample(wt + n2, phase2) * (uint32_t)eg2.output) >> 17);
		
		phase += phase_step;
		phase2 += phase_step2;
		
		if (t == 0) dt = 1;
		else if (t == 65535) dt = -1;
		t += dt;
		
		// Wait for sent flag and clear it
		while (!dac_sent);
		dac_sent = 0;
	}
}
