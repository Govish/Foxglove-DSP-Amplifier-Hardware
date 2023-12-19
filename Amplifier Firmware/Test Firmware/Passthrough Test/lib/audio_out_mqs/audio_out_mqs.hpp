#pragma once

/*
 * By Ishaan Gov December 2023
 * 
 * static class to interact with MQS peripheral on the Teensy 4.1 as relevant to the Foxglove DSP Amplifier project
 * code heavily borrows from "output_mqs" in the Teensy audio library by PJRC
 * 
 * Modifications are made to the interface as well as some slight performance and ease-of-use improvements
 * At the cost of flexibility and portability
 * 
 * Effort is made to make the code as "C++-like" as possible, but I'm pretty new to 
 */


#include <array> //std::array for DMA buffer

#include <Arduino.h>
#include <DMAChannel.h> //we'll be doing all the streaming over DMA

#include "utils.hpp" //for callback functions
#include "config.hpp" //for constants 

class Audio_Out_MQS {
public:

    //use a double-buffered DMA; want to be able to load one half of the buffer while the other half is playing
    //provide a union of two arrays and a larger array to allow for easy interfacing 
    union Audio_Out_DMA_Mem
    {
        struct Buffer_Halves {
            std::array<int32_t, App_Constants::PROCESSING_BLOCK_SIZE> fronthalf;
            std::array<int32_t, App_Constants::PROCESSING_BLOCK_SIZE> backhalf;
        } half_buffers;
        std::array<uint32_t, 2*App_Constants::PROCESSING_BLOCK_SIZE> dma_buffer;
    };
    //ensure our entire buffer is exactly the size we expect
    static_assert(sizeof(Audio_Out_DMA_Mem) == (2*App_Constants::PROCESSING_BLOCK_SIZE*sizeof(uint32_t)));
    

    //implementing with all static methods in order to reduce any chances of hardware ownership issues
    //thus eliminate all types of function that can create class instances
    Audio_Out_MQS() =  delete; //delete constructor
    Audio_Out_MQS(const Audio_Out_MQS&) = delete; //delete copy constructor 
    void operator=(const Audio_Out_MQS&) = delete; //and delete assignment operator

    //initialize hardware peripherals 
    static void init();

    //start the actual operation of the audio output
    static void start();

    //write new audio out data to the peripheral
    //function will automatically route it to the right place in the DMA buffer
    //would like to use a std::span here but need C++17 for that, so using a std::array reference instead
    static void update(std::array<int16_t, App_Constants::PROCESSING_BLOCK_SIZE>& block_in);

    //have some kinda function to call every half-DMA-buffer cycle
    //basically this should reschedule the ADC reading
    //then run the effects chain at a slightly lower priority
    //allowing this to have a generic "context" to run with --> allows this function to be hooked up to a class
    static void attach_interrupt(Context_Callback_Function<void> _user_cb, uint8_t priority);


private:
    //function that gets called when DMA transfers are half-complete / complete
    static void mqs_isr();

    //function that calls the user callback function from a lower interrupt priority level
    //can't call the user callback directly, because it may have some context we need to pass along
    static void user_callback_isr();

    //function that configures clocking to the Serial Audio Interface (SAI) peripheral
    //mostly lifted from `output_mqs.cpp` in the audio library
    static void mqs_configure_clocks();

    //own a DMA channel that services the MQS peripheral (SAI3)
    static DMAChannel mqs_dma;

    //have a piece of memory that the SAI3 peripheral reads from
    //needs to be placed in a specific part of memory
    //this data alignment has to deal with caching (see some relevant DMA-related posts on this forum page)
    //https://forum.pjrc.com/index.php?threads/t4-memory-to-memory-using-dma.69845/
    static DMAMEM __attribute__((aligned(32))) Audio_Out_DMA_Mem dma_memory;
    static bool dma_mem_write_to_fronthalf; //and a flag that directs which part of the DMA buffer to write to

    //and own a callback function that gets called when the DMA requests are half-complete, i.e. we need more data to process
    //making this a Context_Callback_Function to allow this to easily hook up to an instance of a particular class
    static Context_Callback_Function<void> user_cb;
    
};