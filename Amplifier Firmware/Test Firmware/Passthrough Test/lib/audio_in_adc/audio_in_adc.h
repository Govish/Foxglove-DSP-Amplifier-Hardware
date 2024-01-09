#pragma once

#include <array> //for STL array interface
#include <Arduino.h> //for types, interface
#include <DMAChannel.h> //for DMA channel class

#include <config.h> //configuration values

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

    //use a double-buffered DMA; want to be able to read from one half of the buffer while the other half is updating
    //provide a union of two arrays and a larger array to allow for easy interfacing 
    //ADC data is 16-bit unsigned
    union Audio_In_DMA_Mem
    {
        struct Buffer_Halves {
            std::array<uint16_t, App_Constants::PROCESSING_BLOCK_SIZE> fronthalf;
            std::array<uint16_t, App_Constants::PROCESSING_BLOCK_SIZE> backhalf;
        } half_buffers;
        std::array<uint16_t, 2*App_Constants::PROCESSING_BLOCK_SIZE> dma_buffer;
    };
    //ensure our entire buffer is exactly the size we expect
    static_assert(sizeof(Audio_In_DMA_Mem) == (2*App_Constants::PROCESSING_BLOCK_SIZE*sizeof(uint16_t)));

    //implementing with all static methods in order to reduce any chances of hardware ownership issues
    //thus eliminate all types of function that can create class instances
    Audio_In_ADC() =  delete; //delete constructor
    Audio_In_ADC(const Audio_In_ADC&) = delete; //delete copy constructor 
    void operator=(const Audio_In_ADC&) = delete; //and delete assignment operator

    //initialize hardware peripherals 
    static void init();

    //start the actual operation of the audio input
    static void start();

    //get a block of samples from the ADC
    //intended to be called at a rate of SAMPLING_FREQUENCY / PROCESSING_BLOCK_SIZE
    //will copy values into passed into the function
    static void __attribute__((optimize("-O3"))) //hopefully compiler can use some DSP instructions and efficient copies for here
    get_samples(Audio_Block_t& block_out);
    
    //own a DMA channel that services the ADC_ETC peripheral
    static DMAChannel adc_dma;

    //have a piece of memory that the ADC data gets dropped into
    //needs to be placed in a specific part of memory
    //this data alignment has to deal with caching (see some relevant DMA-related posts on this forum page)
    //https://forum.pjrc.com/index.php?threads/t4-memory-to-memory-using-dma.69845/
    static DMAMEM __attribute__((aligned(32))) Audio_In_DMA_Mem dma_memory;
};