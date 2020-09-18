--- Common
100 [max = 1] LFO sync to key
101 [max = 1] LFO reset
102 [max = 1, def = 1] Poly mode
103 Cutoff
111 Debug
104 [min = 1, max = 8, def = 1] Cluster size
105 [max = 7] ID in cluster

--- Oscillator 1
[cc = 20, max = 28, slider = 0] Wavetable
22 [def = 64] Base wave
28 [def = 64] Pitch
24 [def = 64] Detune

--- AMP 1
30 Attack
32 Sustain
34 Release
36 [slider = 0, max = 1] Sustain enabled

--- EG 1
40 Attack
42 Sustain
44 Release
46 [slider = 0, max = 1] Sustain enabled
48 [def = 64] Modulation intensity
# 50 Pitch modulation

--- LFO 1
60 Rate
64 Fade
66 [def = 64] Modulation intensity
# 68 Pitch modulation
62 [max = 1, slider = 0] Wave

--- Oscillator 2
[cc = 21, max = 28, slider = 0] Wavetable
23 [def = 64] Base wave
29 [def = 64] Pitch
25 [def = 64] Detune

--- AMP 2
31 Attack
33 Sustain
35 Release
37 [slider = 0, max = 1] Sustain enabled

--- EG 2
41 Attack
43 Sustain
45 Release
47 [slider = 0, max = 1] Sustain enabled
49 [def = 64] Modulation intensity
# 50 Pitch modulation

--- LFO 2
61 Rate
65 Fade
67 [def = 64] Modulation intensity
# 69 Pitch modulation
63 [max = 1, slider = 0] Wave