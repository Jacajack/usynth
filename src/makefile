F_CPU = 20000000UL
F_SAMPLE = 28000
MIDI_BAUD = 31250
MIDI_MAX_VOICES = 16
MCU = atmega328p
PROGRAMMER = usbasp

CC = avr-gcc
OBJDUMP = avr-objdump
DEFINES = -DF_CPU=$(F_CPU) -DMIDI_BAUD=$(MIDI_BAUD) -DF_SAMPLE=$(F_SAMPLE) -DMIDI_MAX_VOICES=$(MIDI_MAX_VOICES)
CFLAGS = $(DEFINES) -mmcu=$(MCU) -O2 -funroll-loops -g -fdata-sections -ffunction-sections -Wl,--gc-sections -fomit-frame-pointer -faggressive-loop-optimizations -flto -mrelax -Wall

SOURCES = usynth.c midi.c midi_program.c data/notes_table.c data/env_table.c ppg/ppg_data.c ppg/ppg.c
OBJECTS = $(patsubst %.c,%.o,$(SOURCES))
DEPENDS = $(patsubst %.c,%.d,$(SOURCES))

.PHONY: all clean

all: usynth.elf usynth.lss

clean:
	-rm -f usynth.elf usynth.lss $(OBJECTS) $(DEPENDS)

usynth.elf: $(OBJECTS)
	$(CC) $(CFLAGS) -Xlinker -Map=usynth.map $^ -o $@ 
	avr-size -C --mcu=$(MCU) $@

prog: usynth.elf
	avrdude -c $(PROGRAMMER) -p $(MCU) -U flash:w:$^ -U eeprom:w:$^

-include $(DEPENDS)

%.o: %.c makefile
	$(CC) $(CFLAGS) -MMD -MP -c $< -o $@

%.lss: %.elf
	$(OBJDUMP) -drwCSg $< > $@