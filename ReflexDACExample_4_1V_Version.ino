/********************************************************************************************************
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
********************************************************************************************************/

#include <SPI.h>

#define DAC_14_BIT 16383
#define REF_VOLT (float)4.096

//Sets DAC's output mode, only uncomment one at a time
//#define MODE_OUTPUT_SINGLE_VALUE //outputs single DAC voltage level
#define MODE_OUTPUT_FULL_RANGE //runs DAC through is full output voltage range, either low to high or high to low
//#define MODE_OUTPUT_SINE_WAVE //Outputs a sinewave from the DAC, note sinewave will contain artifacts or steps from DAC sample rate frequency
//#define MODE_OUTPUT_QUICK_CHANGE //Toggles DAC's output from max low value to max high value so you can see DAC's slew rate speed

/* ---------- Chip Select pin (use hardware SS by default; override if needed) ---------- */
#ifndef PIN_CS
#define PIN_CS SS        // DAC SYNC̅ / CS pin
#endif

/* ---------- Sine-wave parameters ---------- */
constexpr uint16_t TABLE_SIZE   = 16;        // samples per period for sinewave only

/* ---------- Single Value parameters ---------- */
float outputThisVoltage = -1.000;

/* ---------- Globals ---------- */
// Set SPI clock rate, bit order, and mode 
// (Library will clamp to each MCU's supported maximum.)
SPISettings dacSPI(50000000UL, MSBFIRST, SPI_MODE0);   
uint16_t sineLUT[TABLE_SIZE]; //define the buffer size of sinewave, more points smoother waveform but lower frequency
uint16_t tableCounter = 0; //used for the sine wave generator

void setup()
{
    pinMode(PIN_CS, OUTPUT); //set up CS pin
    digitalWrite(PIN_CS, HIGH);  // default to high
   
    #if defined(MODE_OUTPUT_SINE_WAVE)
        buildLUT(TABLE_SIZE, DAC_14_BIT); //build sine wave
    #endif

    //start SPI communication and settings. We will control CS pin manually for speed
    SPI.begin();                      // use the board's default hardware SPI pins
    SPI.beginTransaction(dacSPI);

    //Compiler directive if statement, runs single output voltage settings. Range is +vref to -vref
    #if defined(MODE_OUTPUT_SINGLE_VALUE) //output a single voltage value
        uint16_t dacCode = dacBipolarVoltageToCode(DAC_14_BIT, REF_VOLT, outputThisVoltage); //convert voltage between 4.096V to -4.096V to 14 bit DAC code
        digitalWrite(PIN_CS, LOW);            // Set CS pin LOW
        SPI.transfer16(dacCode);              // set DAC output value
        digitalWrite(PIN_CS, HIGH);           // Set CS pin HIGH
    #endif
}

void loop()
{
    //Compiler directive if statement, runs sinewave output. Sinewave will appear with "stair steps" with no output filter
    #if defined(MODE_OUTPUT_SINE_WAVE)
        digitalWrite(PIN_CS, LOW);                          // Set CS pin LOW
        SPI.transfer16(sineLUT[tableCounter]);              // send sinewave point to Reflex DAC
        digitalWrite(PIN_CS, HIGH);                         // Set CS pin HIGH
        tableCounter++; //counter for full sine wave
        if(tableCounter >= TABLE_SIZE) tableCounter = 0;    //reset for new sine wave cycle
    #endif

    //Compiler directive if statement, cycles through full range of DAC. 
    //input argument true to count up and false to count down
    #if defined(MODE_OUTPUT_FULL_RANGE)
        fullRangeTest(true,DAC_14_BIT); //function loops through full output range of DAC
    #endif

    //Compiler directive if statement, cycles DAC ouput from max to min. appears as squarewave
    #if defined(MODE_OUTPUT_QUICK_CHANGE)
        quickChangeTest(DAC_14_BIT); //function loops between DAC max range points (+4.096V to -4.096V) as fast as possible
    #endif
}


//runs through full DAC range, can run for low to high or high to low
//input arguments are order of counting (true for up and false for down) and DAC bit count
void fullRangeTest(bool cntUp, uint16_t bits) {
    if(cntUp) {
        for(uint16_t j=0; j < (bits+1); j++) {
            digitalWrite(PIN_CS, LOW);        // Set CS pin LOW
            SPI.transfer16(j);                // count up
            digitalWrite(PIN_CS, HIGH);       // Set CS pin HIGH
        }
    } 
    else {
        for(uint16_t j=0; j < (bits+1); j++) {
            digitalWrite(PIN_CS, LOW);        // Set pin LOW
            SPI.transfer16(bits - j);         //count down
            digitalWrite(PIN_CS, HIGH);       // Set pin HIGH
        }
    }
}

//jumps back and forth from max and min DAC points as fast as possible
//with filtering caps (C24 and C25) in place output resemebles sawtooth waveform
//if you remove filter caps output will look more like a squarewave
void quickChangeTest(uint16_t bits) {
    bool val = false;
    while(1) {
        digitalWrite(PIN_CS, LOW);            // Set CS pin LOW
        if(val) {
            SPI.transfer16(bits);             //DAC max upper range code (+4.096V)
            val = false;
        }
        else {
            SPI.transfer16(0);                //DAC max lower range code (-4.096V)
            val = true;
        }
        digitalWrite(PIN_CS, HIGH);           // Set CS pin High 
    }
}

//this function builds a sinewave and stores it in a global array
//input arguments are DAC bit size and table size
void buildLUT(uint16_t tableSize, uint16_t bits) {
    for (uint16_t i = 0; i < tableSize; ++i)
    {
        float theta   = (2.0f * PI * i) / tableSize; //build sineware input argument using table size
        float amp     = sinf(theta) * 0.5f + 0.5f;   //compute sinewave value from 1 to -1
        uint16_t code = static_cast<uint16_t>(amp * (float)bits + 0.5f); //convert sinwave value to DAC code based on bit size, add 0.5 for integer rounding
        sineLUT[i]    = code & 0x3FFF;              // bits 15-14 = 0
    }
}

//this function takes a bipolar voltage value that is between +Vref and -Vref 
//and converts it to a DAC code that it returns
//input arguments: DAC bit count (14), DAC ref voltage, 
uint16_t dacBipolarVoltageToCode(uint8_t bits, float vref, float vout)
{
  if (bits == 0) return 0;
  if (bits > 14) bits = 14;

  float VREF = (vref >= 0.0f) ? vref : -vref;
  if (VREF == 0.0f) return 0;

  if (vout >  VREF) vout =  VREF;
  if (vout < -VREF) vout = -VREF;

  uint32_t halfScale = (uint32_t)1u << (bits - 1);
  float code_f = (vout / VREF) * (float)halfScale + (float)halfScale;

  int32_t code_i = (int32_t)(code_f + 0.5f);              // no <math.h>
  int32_t maxCode = (int32_t)(((uint32_t)1u << bits) - 1u);
  if (code_i < 0)       code_i = 0;
  if (code_i > maxCode) code_i = maxCode;

  return (uint16_t)code_i;
}
