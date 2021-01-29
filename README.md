# usynth

A scalable, polyphonic 8-bit AVR synthesizer employing wavetable synthesis.

<img src=img/top.png />
<img src=img/bot.png />

Features:
 - PPG Wave 2.2 wavetables
 - 2 wavetable oscillators - independent voices or a single richer one
 - 2 ASR envelope generators per voice - gain, pitch and waveform modulation
 - 1 LFO per voice - triangle or square, with fade, pitch and waveform modulation
 - Single 1-pole lowpass filter
 - MIDI pitch bend support
 - MIDI velocity sensitivity

MIDI controller mappings can be found [here](https://github.com/Jacajack/usynth/blob/master/midictl/usynth-midictl.ctl). If you're on Linux, you can load that file into [midictl](https://github.com/Jacajack/midictl) and use it to control µsynth right away.

## Cluster operation

Multiple µsynth modules can be grouped in a cluster in order to provide richer polyphony.
This hasn't been done yet, but is already implemented in the source code and _should work_&trade;.

## A bit of technical background

The microcontroller is running at 20MHz (max allowed frequency at 5V). By default it's programmed to work with MCP4921 - a 10-bit SPI DAC and output samples at frequency of 28kHz. MIDI commands are received via UART0 at 31250 baud. Pins PD2, PD3 and PD4 are meant to drive status LEDs.

The code is written _mostly_ in C. There are some bits in the AVR assembly language responsible for more sophisticated multiplication, but that's it. I won't go into much detail here, because I think the source code is documented quite well.

The main loop is synced with timer interrupt and generates one sample per iteration. Since there are too many parameters to update all of them every sample, this work is distributed evenly accross each 21 samples. Splitting all the 'slow' code into equal pieces can be quite tricky to get right, but is definitely worth it, since it allows maximal processor time utilization.

Perhaps the most interesting bit is the synthesis process itself. The µC has waveform and wavetable data stored in flash in the same way as they were in PPG Wave. Since the AtMega328 doesn't have enough RAM to hold all the interpolated waveforms at once, everything has to be generated on the fly. In fact, 





<img src=img/voice.png />


## Donate

If you like µsynth and want to support my future projects, you can buy me a cup of coffee :)

[![Donate](https://img.shields.io/badge/Donate-PayPal-green.svg)](https://www.paypal.com/donate?hosted_button_id=KZ7DV93D98GAL)

Thanks! :heart:
