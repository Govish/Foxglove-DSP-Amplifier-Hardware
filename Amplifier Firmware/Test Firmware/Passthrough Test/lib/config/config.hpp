#pragma once

/*
 * By Ishaan Govindarajan December 2023
 *
 * Having definitions of app-wide constants here
 * It would be nice to not need a file like this (i.e. use some flavor of templatization when creating classes and such)
 *      But templatization requires all methods to be implemented in the header file which feels kinda broken and bad practice
 * 
 * As such, defining a file with constants in its own namespace; will include these in other translation units as required
 */

#include <Arduino.h> //for types and stuff

namespace Pindefs {
    //RGB LED pin mapping
    constexpr uint8_t LED_CHAN1_R = 0;
    constexpr uint8_t LED_CHAN1_B = 2;
    constexpr uint8_t LED_CHAN1_G = 1;
    constexpr uint8_t LED_CHAN2_R = 3;
    constexpr uint8_t LED_CHAN2_B = 5;
    constexpr uint8_t LED_CHAN2_G = 4;
    constexpr uint8_t LED_CHAN3_R = 6;
    constexpr uint8_t LED_CHAN3_B = 8;
    constexpr uint8_t LED_CHAN3_G = 7;
    constexpr uint8_t LED_CHAN4_R = 9;
    constexpr uint8_t LED_CHAN4_B = 24;
    constexpr uint8_t LED_CHAN4_G = 11;
    constexpr uint8_t LED_MAIN_R = 25;
    constexpr uint8_t LED_MAIN_B = 29;
    constexpr uint8_t LED_MAIN_G = 28;
    constexpr bool RGB_ACTIVE_HIGH = false; //RGB LEDs are active LOW

    //Encoder pin mapping
    constexpr uint8_t ENC_CHAN1_A = 33;
    constexpr uint8_t ENC_CHAN1_B = 34;
    constexpr uint8_t ENC_CHAN1_SW = 32;
    constexpr uint8_t ENC_CHAN2_A = 35;
    constexpr uint8_t ENC_CHAN2_B = 36;
    constexpr uint8_t ENC_CHAN2_SW = 31;
    constexpr uint8_t ENC_CHAN3_A = 37;
    constexpr uint8_t ENC_CHAN3_B = 38;
    constexpr uint8_t ENC_CHAN3_SW = 30;
    constexpr uint8_t ENC_CHAN4_A = 39;
    constexpr uint8_t ENC_CHAN4_B = 40;
    constexpr uint8_t ENC_CHAN4_SW = 27;
    constexpr uint8_t ENC_MAIN_A = 41;
    constexpr uint8_t ENC_MAIN_B = 17;
    constexpr uint8_t ENC_MAIN_SW = 26;

    //Input level indicator pin mapping
    constexpr uint8_t LEVEL_LOW = 10;
    constexpr uint8_t LEVEL_MED = 13;
    constexpr uint8_t LEVEL_HIGH = 14;
    constexpr uint8_t LEVEL_CLIP = 15;
    constexpr bool LEVEL_ACTIVE_HIGH = false; //LEDs here are active LOW
    
    //ADC pin and channel for sampling
    //pin corresponds to the Arduino pin; channel corresponds to the ADC2 channel
    constexpr uint8_t INPUT_ADC_PIN = A2;
    constexpr uint32_t INPUT_ADC_CHANNEL = 12;

    //output is hardcoded on pin 12 --> has to be mapped to an MQS pin (other option is 10, change in `audio_output_mqs.cpp`)
}

namespace App_Constants {
    //instead of processing a single sample at a time
    //firmware will process a "block" of data, similar to audio library
    //this constant sets how big those blocks are
    //highly recommend keeping this as a multiple of 16 --> caching behavior is a little muddy
    //with non-32-byte memory chunks (each block element is an int16_t hence multiple of 16, not 32)
    constexpr size_t PROCESSING_BLOCK_SIZE = 128;

    //operating frequencies and ratios
    constexpr uint32_t AUDIO_SAMPLE_RATE_HZ = 48000;
    constexpr uint32_t MQS_PWM_FREQ_HZ = AUDIO_SAMPLE_RATE_HZ * 8; //approximately the factory configuration 
    constexpr uint32_t MQS_OVERSAMPLE_RATE = 64; //should give a little better performance?

    //interrupt priorities
    constexpr uint8_t MQS_DMA_INT_PRIO = 10;
    constexpr uint8_t AUDIO_BLOCK_PROCESS_PRIO = 20;
    constexpr uint8_t ENC_SAMPLING_PRIO = 30;

    //encoder and switch bounce time (seconds)
    //switch algorithm will sample switches at this frequency
    //will technically be approximate, but not a precision timing application
    constexpr float ENC_BOUNCE_TIME = 2.5e-3f;
    constexpr float ENC_SW_BOUNCE_TIME = 15e-3f; //will run off the same interrupt, just sample less frequently
    
    //how many encoder instances we'll be running total
    //this is so we don't need a growable container (and therefore heap allocation) to store all encoder instances
    constexpr size_t NUM_ENCODERS = 5;
};

namespace Audio_Clocking_Constants {
    //audio PLL constants; this will set the clocking frequency of the Audio PLL output
    //this should be equal to some multiple of MQS_OVERSAMPLE_RATE * MQS_PWM_FREQ_HZ
    // PLL_output_frequency = F_ref * (DIV_SELECT + NUM/DEN), where F_ref is 24MHz (PLL input clock source)
    constexpr uint32_t AUDIO_PLL_NUM = 768;
    constexpr uint32_t AUDIO_PLL_DEN = 1000;
    constexpr uint32_t AUDIO_PLL_DIVSEL = 32;

    //set the SAI3 clock prescalers from the Audio PLL
    //sets the input frequency of the MQS peripheral
    //SAI3 clock frequency is Audio_PLL_output_frequency / (PRESC_1 * PRESC_2)
    constexpr uint32_t SAI3_PRESC_1 = 4;
    constexpr uint32_t SAI3_PRESC_2 = 8;

    //the the clock divider into the I2S3 module
    //sets the bit clock frequency as a fraction of the SAI3 clock frequency
    //I2S3_clock_freq = SAI3_clock_freq / PRESC
    constexpr uint32_t I2S3_PRESC = 16;

    //ADC sampling clock divider from 24MHz
    //this setting informs the PIT channel 0 load value to schedule ADC readings
    //timer will be clocked from a 24MHz source
    constexpr uint32_t ADC_SAMPLING_DIVIDER = 24000000UL / App_Constants::AUDIO_SAMPLE_RATE_HZ;
};

//##############################################################################################################################################
//============================= DO NOT MODIFY ANYTHING BELOW HERE! SANITY CHECK SETTINGS AND COMPUTE REGISTER CONSTANTS ========================
//##############################################################################################################################################

//some variables for sanity check math
static constexpr uint32_t AUDIO_PLL_INPUT_FREQUENCY = 24000000; //Hz; frequency of the clock input to the Audio PLL
static constexpr uint32_t AUDIO_PLL_OUTPUT_FREQUENCY =  (float)AUDIO_PLL_INPUT_FREQUENCY * (float)Audio_Clocking_Constants::AUDIO_PLL_DIVSEL +
                                                        (float)AUDIO_PLL_INPUT_FREQUENCY / (float)Audio_Clocking_Constants::AUDIO_PLL_DEN * (float)Audio_Clocking_Constants::AUDIO_PLL_NUM;
static constexpr uint32_t SAI3_BUS_FREQUENCY = AUDIO_PLL_OUTPUT_FREQUENCY / (   Audio_Clocking_Constants::SAI3_PRESC_1 * 
                                                                                Audio_Clocking_Constants::SAI3_PRESC_2);
static constexpr uint32_t MQS_INPUT_FREQUENCY = App_Constants::MQS_OVERSAMPLE_RATE * App_Constants::MQS_PWM_FREQ_HZ;

static constexpr uint32_t BIT_CLOCK_CYCLES_PER_FRAME = 32; //how many times the bit clock cycles constitute of one MQS L+R data frame
static constexpr uint32_t I2S3_INPUT_FREQUENCY = App_Constants::AUDIO_SAMPLE_RATE_HZ * BIT_CLOCK_CYCLES_PER_FRAME * Audio_Clocking_Constants::I2S3_PRESC;

static_assert(  App_Constants::PROCESSING_BLOCK_SIZE % 16 == 0,
                "HIGHLY recommend to have PROCESSING_BLOCK_SIZE be a multiple of 16");

static_assert(  App_Constants::MQS_OVERSAMPLE_RATE == 32 || App_Constants::MQS_OVERSAMPLE_RATE == 64,
                "MQS_OVERSAMPLE_RATE needs to be 32 or 64!");

static_assert(  App_Constants::MQS_PWM_FREQ_HZ % App_Constants::AUDIO_SAMPLE_RATE_HZ == 0,
                "HIGHLY recommended to have MQS_PWM_FREQ_HZ be a multiple of AUDIO_SAMPLE_RATE_HZ");

static_assert(  MQS_INPUT_FREQUENCY < 66500000,
                "MQS peripheral clock frequency must be less than 66.5MHz!");

static_assert(  Audio_Clocking_Constants::AUDIO_PLL_DIVSEL >= 27,
                "AUDIO_PLL_DIVSEL must be at least 27");

static_assert(  Audio_Clocking_Constants::AUDIO_PLL_DIVSEL <= 54,
                "AUDIO_PLL_DIVSEL must be at most 54");

static_assert(  Audio_Clocking_Constants::AUDIO_PLL_NUM < Audio_Clocking_Constants::AUDIO_PLL_DEN,
                "AUDIO_PLL_NUM must be strictly less than AUDIO_PLL_DEN");

static_assert(  Audio_Clocking_Constants::AUDIO_PLL_NUM < (uint32_t)(1<<31),
                "AUDIO_PLL_NUM value too large");

static_assert(  Audio_Clocking_Constants::AUDIO_PLL_DEN < (uint32_t)(1<<31),
                "AUDIO_PLL_DEN value too large");

static_assert(  I2S3_INPUT_FREQUENCY == MQS_INPUT_FREQUENCY,
                "Clock Divider Mismatch! Change clock dividers of MQS and SAI3 peripherals such that frequencies match up!");

static_assert(  I2S3_INPUT_FREQUENCY == SAI3_BUS_FREQUENCY,
                "Clock Divider Mismatch! Change SAI3/Audio PLL prescalers such that SAI3 bus frequency matches MQS/SAI3 peripheral expected input frequencies!");

static_assert(  Audio_Clocking_Constants::SAI3_PRESC_1 > 0,
                "SAI3_PRESC_1 must be at least 1!");

static_assert(  Audio_Clocking_Constants::SAI3_PRESC_1 <= 8,
                "SAI3_PRESC_1 must be at most 8!");

static_assert(  Audio_Clocking_Constants::SAI3_PRESC_2 > 0,
                "SAI3_PRESC_2 must be at least 1!");

static_assert(  Audio_Clocking_Constants::SAI3_PRESC_2 <= 64,
                "SAI3_PRESC_2 must be at most 64!");

static_assert(  Audio_Clocking_Constants::I2S3_PRESC % 2 == 0,
                "I2S3_PRESC must be an even number!");

static_assert(  Audio_Clocking_Constants::I2S3_PRESC >= 2,
                "I2S3_PRESC must be at least 2!");

static_assert(  Audio_Clocking_Constants::I2S3_PRESC <= 512,
                "I2S3_PRESC must be at most 512!");

static_assert(  24000000.0f / ((float)Audio_Clocking_Constants::ADC_SAMPLING_DIVIDER) == (float)App_Constants::AUDIO_SAMPLE_RATE_HZ,
                "ADC_SAMPLING_DIVIDER and AUDIO_SAMPLE_RATE_HZ mismatch! Correct one of them!");

//check ADC channel
//use the pin mapping below to ensure the ADC channel and MCU pin are the same
//lifted from `input_adc.cpp` in the PJRC Audio Library
PROGMEM static constexpr uint8_t adc2_pin_to_channel[] = {
	7,      // 0/A0  AD_B1_02
	8,      // 1/A1  AD_B1_03
	12,     // 2/A2  AD_B1_07
	11,     // 3/A3  AD_B1_06
	6,      // 4/A4  AD_B1_01
	5,      // 5/A5  AD_B1_00
	15,     // 6/A6  AD_B1_10
	0,      // 7/A7  AD_B1_11
	13,     // 8/A8  AD_B1_08
	14,     // 9/A9  AD_B1_09
	255,	// 10/A10 AD_B0_12 - only on ADC1, 1 - can't use for audio
	255,	// 11/A11 AD_B0_13 - only on ADC1, 2 - can't use for audio
	3,      // 12/A12 AD_B1_14
	4,      // 13/A13 AD_B1_15
	7,      // 14/A0  AD_B1_02
	8,      // 15/A1  AD_B1_03
	12,     // 16/A2  AD_B1_07
	11,     // 17/A3  AD_B1_06
	6,      // 18/A4  AD_B1_01
	5,      // 19/A5  AD_B1_00
	15,     // 20/A6  AD_B1_10
	0,      // 21/A7  AD_B1_11
	13,     // 22/A8  AD_B1_08
	14,     // 23/A9  AD_B1_09
	255,    // 24/A10 AD_B0_12 - only on ADC1, 1 - can't use for audio
	255,    // 25/A11 AD_B0_13 - only on ADC1, 2 - can't use for audio
	3,      // 26/A12 AD_B1_14 - only on ADC2, do not use analogRead()
	4,      // 27/A13 AD_B1_15 - only on ADC2, do not use analogRead()
#ifdef ARDUINO_TEENSY41
	255,    // 28
	255,    // 29
	255,    // 30
	255,    // 31
	255,    // 32
	255,    // 33
	255,    // 34
	255,    // 35
	255,    // 36
	255,    // 37
	1,      // 38/A14 AD_B1_12 - only on ADC2, do not use analogRead()
	2,      // 39/A15 AD_B1_13 - only on ADC2, do not use analogRead()
	9,      // 40/A16 AD_B1_04
	10,     // 41/A17 AD_B1_05
#endif
};

static_assert(  adc2_pin_to_channel[Pindefs::INPUT_ADC_PIN] == Pindefs::INPUT_ADC_CHANNEL,
                "Mismatch between ADC pin and channel!");

static_assert(  App_Constants::ENC_SW_BOUNCE_TIME >= App_Constants::ENC_BOUNCE_TIME,
                "Encoder switch bounce time must be greater than or equal to rotation bounce time!");