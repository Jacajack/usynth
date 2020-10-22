#!/bin/bash

for i in $(seq 1 $#); do
	echo "MIDI_PROGRAM_BEGIN($i),";
	cat "${!i}"
done
