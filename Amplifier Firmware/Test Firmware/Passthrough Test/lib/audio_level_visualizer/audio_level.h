#pragma once

/**
 * This is a quick module that visualizes the input audio level
 * Basically implemented as a peak detector with an exponential decay
 * Whose decay rate is set by the configuration constant 
 * Intended to be used statically in order to minimize any weird hardware discrepancies
 * 
 * Basic operation of this module is to emulate a positive peak detector circuit with a decay resistor
 * Similar to the "Positive Peak Detector" section of the following circuit:
 * https://www.researchgate.net/publication/336268955/figure/fig10/AS:810482210463745@1570245425449/Positive-and-negative-peak-detector-circuits.jpg
 * 
 * NOTE:    using 32-bit formats for all the memory elements since the long decay times of the filter
 *          require a decent bit of precision.
 * 
 * By Ishaan Gov Jan 2024
*/

#include <array>
#include <limits>
#include <Arduino.h>

#include <config.h>
#include <dspinst.h> //for SIMD instruction for speed and such

class Audio_Level_Vis {
public:
    //delete all the constructors and assignment operators
    Audio_Level_Vis() = delete;
    Audio_Level_Vis(const Audio_Level_Vis& other) = delete;
    void operator=(const Audio_Level_Vis& other) = delete;
    
    //initialize the hardware, memory elements, and whatever else 
    static void init();

    //run the visualizer; nominally called within the audio update
    //take a reference as not to waste time copying 
    static void update(const Audio_Block_t& block_in);

private:
    //maintain a list of LED pins we'll use for our visualizer
    static const std::array<uint8_t, App_Constants::LEVEL_VIS_NUM_LEDS> led_pins;

    //maintain a list of threshold levels for our visualizer (including hysteresis)
    static std::array<int32_t, App_Constants::LEVEL_VIS_NUM_LEDS> high_thresholds;
    static std::array<int32_t, App_Constants::LEVEL_VIS_NUM_LEDS> low_thresholds;

    //memory variables for our visualizer
    static volatile int32_t peak_memory;    //our peak value we'll be using for our visualizer
    static volatile int32_t peak_decay;     //in Q0.32 format for what our exponential decay parameter should be
};