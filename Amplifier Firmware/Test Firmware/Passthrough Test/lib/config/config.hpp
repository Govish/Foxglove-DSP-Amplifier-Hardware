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

namespace App_Constants {
    //instead of processing a single sample at a time
    //firmware will process a "block" of data, similar to audio library
    //this constant sets how big those blocks are
    constexpr size_t PROCESSING_BLOCK_SIZE = 128;

    //operating frequencies and ratios
    constexpr uint32_t AUDIO_SAMPLE_RATE_HZ = 48000;
    constexpr uint32_t MQS_PWM_FREQ_HZ = AUDIO_SAMPLE_RATE_HZ * 8; //approximately the factory configuration 
    constexpr uint32_t MQS_OVERSAMPLE_RATE = 64; //should give a little better performance?

    //interrupt priorities
    constexpr uint8_t MQS_DMA_INT_PRIO = 10;
    constexpr uint8_t AUDIO_BLOCK_PROCESS_PRIO = 20;

    //ADC channel for sampling
    //NOTE: this is not the arduino pin number! It's the ADC channel with respect to ADC2
    constexpr uint32_t INPUT_ADC_CHANNEL = 12; //pin A2

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