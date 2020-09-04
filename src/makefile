F_CPU = 20000000UL
F_SAMPLE = 28000
MIDI_BAUD = 31250
MCU = atmega328p

CC = avr-gcc
OBJDUMP = avr-objdump
CFLAGS = -DF_CPU=$(F_CPU) -DMIDI_BAUD=$(MIDI_BAUD) -DF_SAMPLE=$(F_SAMPLE) -mmcu=$(MCU) -O3 -funroll-loops -flto -Wall

SOURCES = usynth.c midi.c notes.c data/mod_table.c data/env_table.c ppg/ppg_data.c ppg/ppg.c
OBJECTS = $(patsubst %.c,%.o,$(SOURCES))
DEPENDS = $(patsubst %.c,%.d,$(SOURCES))

.PHONY: all clean

all: usynth.elf usynth.lss

clean:
	-rm -f usynth.elf $(OBJECTS) $(DEPENDS)

usynth.elf: $(OBJECTS)
	$(CC) $(CFLAGS) $^ -o $@
	avr-size -C --mcu=$(MCU) $@

prog: usynth.elf
	avrdude -c usbasp -p m328p -U flash:w:usynth.elf

-include $(DEPENDS)

%.o: %.c makefile
	$(CC) $(CFLAGS) -MMD -MP -c $< -o $@

%.lss: %.elf
	$(OBJDUMP) -h -S $< > $@