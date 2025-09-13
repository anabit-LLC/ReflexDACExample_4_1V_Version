# ReflexDACExample_4_1V_Version
This example Arduino sketch is meant to work with Anabit's Reflex DAC 14 Bit 2MSPS bipolar output +/- 4.096 
open source reference design

Product link: https://anabit.co/products/reflex-dac-14-bit-2msps-4v-range/

This example sketch demonstrates how to output 4 different voltage patterns from the 14 bit DAC. 
This portable version targets ANY Arduino board with hardware SPI and uses standard digitalWrite()
for the chip select (CS) pin. The DAC is driven over SPI using the board's default SPI pins.

The sketch can be used to create the following 4 outputs: 
--> single voltage value (between -4.096V and +4.096V)
--> Ramp waveform that cycles through all of the DAC points 2^14 (-4.096V to 4.096V)
--> Sinewave waveform, the number of points defined per cylce and SPI clock speed determine
output frequency of the sinewave. Note when you view the sinewave on an oscillscope, please 
note you will see artifacts from the DAC's sample rate frequency (look like steps)
--> Quick change waveform (will look like saw tooth waveform) toggles points from +4.096 to
-4.096 as fast as possible. If you remove filter caps C24 and C25 this will appear more as a
squarewave

Note: some boards (such as ESP32) perform background tasks that can make loop timing inconsistent, which may cause 
minor timing jitter in the sinewave and quick-change outputs.

Please report any issue with the sketch to the Anagit forum: 
https://anabit.co/community/forum/digital-to-analog-converters-dacs

Example code developed by Your Anabit LLC © 2025
Licensed under the Apache License, Version 2.0.
