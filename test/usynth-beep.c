#define F_CPU 20000000UL

#include <util/delay.h>
#include <avr/io.h>

#include <inttypes.h>

#define LDAC_PIN 1
#define CS_PIN   2
#define MOSI_PIN  3
#define SCK_PIN  5

void spi_txb(uint8_t b)
{
	SPDR = b;
	while (!(SPSR & (1 << SPIF)));
}

#define MCP4921_DAC_AB_BIT   (1 << 15)
#define MCP4921_VREF_BUF_BIT (1 << 14)
#define MCP4921_GAIN_BIT     (1 << 13)
#define MCP4921_SHDN_BIT     (1 << 12)

void mcp4921_write(uint16_t data)
{
	PORTB &= ~(1 << CS_PIN);
	spi_txb(data >> 8);
	spi_txb(data);
	PORTB |= (1 << CS_PIN);
}

int main(void)
{
	// LEDs
	DDRD |= (1 << 2) | (1 << 3) | (1 << 4);
	
	// LDAC, CS hi
	PORTB |= (1 << LDAC_PIN) | (1 << CS_PIN);
	
	// DAC
	DDRB |= (1 << LDAC_PIN) | (1 << CS_PIN) | (1 << MOSI_PIN) | (1 << SCK_PIN);
	
	// SPI init
	SPCR = (1 << SPE) | (1 << MSTR);//| (1 << SPR0);
	SPSR = (1 << SPI2X);
	
	// Tie LDAC low, to update on CS
	PORTB &= ~(1 << LDAC_PIN);
	
	uint16_t mcp4921_conf = MCP4921_SHDN_BIT | MCP4921_GAIN_BIT | MCP4921_VREF_BUF_BIT;
	
	while (1)
	{
		for (int i = 0; i < 4096; i += 16)
		{
			int x = (i < 2048) * 4095;
			mcp4921_write((x & 4095) | mcp4921_conf);
		}
	}
}
