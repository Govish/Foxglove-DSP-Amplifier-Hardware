#pragma once

#include "config.hpp" //configuration values 

/*
 * By Ishaan Gov December 2023
 * 
 * static class to interact with ADC peripheral on the Teensy 4.1 as relevant to the Foxglove DSP Amplifier project
 * code heavily borrows from "input_adc" in the Teensy audio library by PJRC
 * 
 * Modifications are made to the interface as well as some slight performance and ease-of-use improvements
 * At the cost of flexibility and portability
 * 
 * Effort is made to make the code as "C++-like" as possible, but I'm pretty new to 
 */

class Audio_In_ADC {
public:
    //implementing with all static methods in order to reduce any chances of hardware ownership issues
    //thus eliminate all types of function that can create class instances
    Audio_In_ADC() =  delete; //delete constructor
    Audio_In_ADC(const Audio_In_ADC&) = delete; //delete copy constructor 
    void operator=(const Audio_In_ADC&) = delete; //and delete assignment operator

    //initialize hardware peripherals 
    static void init();

    //start the actual operation of the audio input
    static void start();

private:
};