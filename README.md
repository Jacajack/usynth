# usynth
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![Donate](https://img.shields.io/badge/Donate-PayPal-green.svg)](https://www.paypal.com/donate?hosted_button_id=KZ7DV93D98GAL)

A scalable, polyphonic 8-bit AVR synthesizer employing wavetable synthesis - a summer project that exceeded my expectations.

Features:
 - PPG Wave 2.2 wavetables
 - 2 wavetable oscillators - independent voices or a single richer one
 - 2 ASR envelope generators per voice - gain, pitch and waveform modulation
 - 1 LFO per voice - triangle or square, with fade, provides pitch and waveform modulation
 - Cluster operation to provide greater polyphony (this hasn't been tested yet, but is already implemented in the source code and _should work_&trade;)
 - 1-pole lowpass filter
 - All parameters can be adjusted as MIDI controllers
 - Presets stored in flash memory
 - MIDI pitch bend support
 - MIDI velocity sensitivity
 
MIDI controller mappings can be found [here](https://github.com/Jacajack/usynth/blob/master/midictl/usynth-midictl.ctl). If you're on Linux, you can load that file directly into [midictl](https://github.com/Jacajack/midictl) and use it to control µsynth right away.

See demo on YouTube:<br>
[![Demo](http://img.youtube.com/vi/hL-aASeibNs/0.jpg)](http://www.youtube.com/watch?v=YhL-aASeibNs "µsynth demo")


## A bit of technical background

The microcontroller is running at 20MHz (max allowed frequency at 5V). By default it's programmed to work with MCP4921 - a 10-bit SPI DAC and output samples at frequency of 28kHz. MIDI commands are received via UART0 at 31250 baud. Pins PD2, PD3 and PD4 are meant to drive status LEDs. I won't go into much detail here, because I think the source code is documented quite well.

The code is written _mostly_ in C. There are some bits in the AVR assembly language responsible for more sophisticated multiplication, but that's it. The chip is utilized pretty well with ~99% flash and ~77% RAM usage. 

The main loop is synced with timer interrupt and generates one sample per iteration. Since there are too many parameters to update all of them every sample, this work is distributed evenly accross each 21 samples. Splitting all the 'slow' code into equal pieces can be quite tricky to get right, but is definitely worth it, since it allows maximal processor time utilization.

Perhaps the most interesting bit is the synthesis process itself. The µC has waveform and wavetable data stored in flash in the same way as they were in PPG Wave (see [here](https://jacajack.github.io/music/2019/12/10/PPG-EPROM.html)). Since the AtMega328 doesn't have enough RAM to hold all the interpolated waveforms at once, everything has to be computed on the fly. In fact, I've already described this process on my blog, so I'll just refer you [there](https://jacajack.github.io/music/synths/2020/04/25/More-PPG.html).

The block diagram below represents simplified structure of a single voice. The 2 voices can be used independently or combined as a single richer one.

<img width=60% src=img/voice.png />

## Donate

If you like µsynth and want to support my future projects, you can buy me a cup of coffee below. It will be very appreciated :)

[![Donate](https://img.shields.io/badge/Donate-PayPal-green.svg)](https://www.paypal.com/donate?hosted_button_id=KZ7DV93D98GAL)

Thanks! :heart:
