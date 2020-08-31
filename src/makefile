F_CPU = 20000000UL
F_SAMPLE = 20000
MIDI_BAUD = 31250
MCU = atmega328p

CC = avr-gcc
CFLAGS = -DF_CPU=$(F_CPU) -DMIDI_BAUD=$(MIDI_BAUD) -DF_SAMPLE=$(F_SAMPLE) -mmcu=$(MCU) -O3 -funroll-loops -flto

all: usynth.elf

usynth.elf: usynth.c ppg/ppg_data.c ppg/ppg.c
	$(CC) $(CFLAGS) $^ -o $@

clean:
	-rm usynth.elf
	
prog: usynth.elf
	avrdude -c usbasp -p m328p -U flash:w:usynth.elf
