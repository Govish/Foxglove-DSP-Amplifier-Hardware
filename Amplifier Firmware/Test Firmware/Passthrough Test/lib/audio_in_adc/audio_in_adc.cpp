#include "audio_in_adc.hpp"

#include "imxrt.h" //for register-level control

//========================= STATIC VARIABLE INITIALIZATION =========================

DMAChannel Audio_In_ADC::adc_dma(false); //don't allocate just yet
DMAMEM __attribute__((aligned(32))) Audio_In_ADC::Audio_In_DMA_Mem Audio_In_ADC::dma_memory;

//=========================== PUBLIC MEMBER FUNCTIONS ======================

void Audio_In_ADC::init() {
    /*
     * Configure the Periodic Interrupt Timer (PIT)
     *  Timer interrupt frequency is given by:
     *  PIT_clock/(period + 1) --> adapted from formula on p. 3041 NOTE THE `+1`!
     *  We'll be using >> CHANNEL_0 << to trigger the ADC reads, run this at the audio sample rate
     *  Will likely use other channels for other interrupt and task scheduling around the firmware
     */
    CCM_CSCMR1 |= CCM_CSCMR1_PERCLK_CLK_SEL; //source PIT and GPT clocks from 24MHz oscillator
    CCM_CCGR1 |= CCM_CCGR1_PIT(CCM_CCGR_ON); //enable clock to PIT in all modes
    PIT_MCR = 0; //turn on PIT, enable logic is inverted for whatever reason
    PIT_TCTRL0 = 0; //disable the timer if it's running for whatever reason
    PIT_LDVAL0 = Audio_Clocking_Constants::ADC_SAMPLING_DIVIDER - 1; //set the period of the timer according to config

    /*
     * Configure ADC External Trigger Control 
     *  For whatever reason, this is separate from ADCs kinda generally
     *  But this is where we set up our ADC trigger source
     *  And DMA generation as necessary
     */

    //ensure that ETC isn't in reset state, and that ADC2 can be operational
    if (ADC_ETC_CTRL & (ADC_ETC_CTRL_SOFTRST | ADC_ETC_CTRL_TSC_BYPASS)) {
		ADC_ETC_CTRL = 0; // clears SOFTRST only
		ADC_ETC_CTRL = 0; // clears TSC_BYPASS to enable ADC2 (have to do this according to datasheet, p. 3459)
	}
    //pulsed DMA mode, also first trigger spot for ADC2 (lifted from `input_adc.cpp`)
    const uint32_t ADC2_TRIGGER_CHANNEL = 4; //4-7 for ADC2
    ADC_ETC_CTRL |= ADC_ETC_CTRL_TRIG_ENABLE(1 << ADC2_TRIGGER_CHANNEL) | ADC_ETC_CTRL_DMA_MODE_SEL;
	ADC_ETC_DMA_CTRL |= ADC_ETC_DMA_CTRL_TRIQ_ENABLE(ADC2_TRIGGER_CHANNEL);

    // configure our particular trigger channel to trigger an ADC conversion on our particular ADC channel
	const uint32_t TRIGGER_LENGTH = 1;
	IMXRT_ADC_ETC.TRIG[ADC2_TRIGGER_CHANNEL].CTRL = 
        ADC_ETC_TRIG_CTRL_TRIG_CHAIN(TRIGGER_LENGTH - 1) | ADC_ETC_TRIG_CTRL_TRIG_PRIORITY(7); //highest priority
	IMXRT_ADC_ETC.TRIG[ADC2_TRIGGER_CHANNEL].CHAIN_1_0 = ADC_ETC_TRIG_CHAIN_HWTS0(1) | //drop into ADC2 hardware trigger 0
		ADC_ETC_TRIG_CHAIN_CSEL0(Pindefs::INPUT_ADC_CHANNEL) | ADC_ETC_TRIG_CHAIN_B2B0; //measure our selected channel, no delays

    //now set up our crossbar to trigger ADC_ETC channel 4 from our PIT channel 0
    //NOTE: there's a weird "S" in the XBAR block diagram in the path of the PIT 
    //that i swear to god is explained nowhere. This datasheet is amazing sometimes; proceed with caution
    CCM_CCGR2 |= CCM_CCGR2_XBAR1(CCM_CCGR_ON); //enable clocking to crossbar
    XBARA1_SEL53 = XBARA1_IN_PIT_TRIGGER0 << 8; //HARDCODING PIT channel 0 into trigger 4 of ADC_ETC

    /*
     * Perform ADC configuration at the register level
     *  We have a pretty specialized application, and getting the ADCs to do what we want
     *  Using existing library methods might be more effort than it's worth
     * 
     *  We'll instead configure the ADC (ADC2) at the register level, similar to how it's done
     *  in the audio library
     * 
     * NOTE: total ADC conversion time set by:
     *  t_conv = sfc_adder + average_num * (bct + lst_adder)
     *      single/first continuous time adder --> 4 ADCK cycles + 2 bus clock cycles
     *      average number factor --> 1 - 32x
     *      base conversion time --> 25 ADCK cycles (12-bit)
     *      long sample time adder --> 3 - 25 ADCK cycles
     * 
     * NOTE2: jesus f*cking christ the entire ADC section mentions the ADACK but fails to list the frequency
     *  shoutout to: https://forum.pjrc.com/index.php?threads/adc-async-clock-generation-teensy-4-0.59728/post-230804
     *  forum post for linking the electrical spec sheet where it's listed in one row 
     *  ADACK will tick at 10MHz if ADHSC = 0 and 20MHz if ADHSC is 1
     * 
     */


    //REALLY IMPORTANT! set the audio input pin to INPUT!!!
    //without calling it defaults to this input pull with hysteresis mode, which adds distortion to sources with non-zero output impedance
    //was an interesting bug to track down, but everything sounds good now!
    pinMode(Pindefs::INPUT_ADC_PIN, INPUT);

    //set our averaging, conversion trigger
    //  conversion speed, conversion mode, sample time,
    //  clock division, and clock source
    uint32_t adc_config_reg = 0;
    adc_config_reg |= ADC_CFG_AVGS(1); //8x oversampling
    adc_config_reg |= ADC_CFG_MODE(2); //12-bit conversions
    adc_config_reg |= ADC_CFG_ADTRG; //hardware triggers
    adc_config_reg |= ADC_CFG_ADICLK(3); //use asynchronous ADC clock
    adc_config_reg |= ADC_CFG_ADIV(0); //no input clock division happening
    adc_config_reg |= ADC_CFG_ADHSC; //run high-speed conversions - 20MHz internal clock
    adc_config_reg |= ADC_CFG_ADSTS(0); //long sample time, sample for 13 ADC cycles
    adc_config_reg |= ADC_CFG_ADLSMP; //long sample time
    ADC2_CFG = adc_config_reg;

    //set our conversion trigger source from the ADC external trigger controller (adc_etc)
    //don't need to set the conversion complete interrupt, since it'll be serviced by DMA
    ADC2_HC0 = ADC_HC_ADCH(16); //ADC_ETC

    //enable general control parameters --> hardware averaging, internal clock, DMA?
    ADC2_GC = ADC_GC_AVGE | ADC_GC_ADACKEN /*| ADC_GC_DMAEN */; 

    //calibrate ADCs TODO: FIX!!!
    //ADC2_GC |= ADC_GC_CAL;
    //while(ADC2_GC & ADC_GC_CAL); //wait for calibration to complete
    //if(ADC2_GS & ADC_GS_CALF) while(1); //hang if calibration failed TODO: fail gracefully

    /*
     * Configure our DMA
     *  Instead of triggering off the ADC peripheral, DMA hooks up to the ADC_ETC peripheral
     *  This is more or less following the model provided by `input_adc.cpp`
     *  Using the DMA library to do most of the nitty-gritty configuration
     */

    adc_dma.begin();

    //pull the ADC data from the ADC trigger controller, rather than the ADC itself
    //this is how it's done in `input_adc.cpp` at least
    adc_dma.source((volatile uint16_t &)IMXRT_ADC_ETC.TRIG[4].RESULT_1_0);

    //put the adc readings into our double buffered structure
    //our data starts at the starting address of the DMA buffer
    adc_dma.destinationBuffer((volatile uint16_t *) &dma_memory.dma_buffer, sizeof(dma_memory.dma_buffer));
    
    //transfer 2 bytes at a time --> readings are 16-bit, right justified
    adc_dma.transferSize(2);

    //and trigger DMA with the ADC ETC peripheral
    //rather than the ADC conversion complete itself
    adc_dma.triggerAtHardwareEvent(DMAMUX_SOURCE_ADC_ETC);
}

void Audio_In_ADC::start() {
    //enable DMA
    adc_dma.enable();

    //enable PIT channel 0
    PIT_TCTRL0 = PIT_TCTRL_TEN;
}

void Audio_In_ADC::get_samples(std::array<int16_t, App_Constants::PROCESSING_BLOCK_SIZE>& block_out) {
    //need to ensure the address of the memory buffer is flushed from cache
    //DMA will dump directly to RAM (RAM2), need to ensure processor accesses ram (and not cache)
    arm_dcache_delete(&dma_memory.dma_buffer, sizeof(dma_memory.dma_buffer));
    
    //get the current DMA destination address (RAM2) and the halfway point in our DMA memory chunk
	uint32_t dma_active_address = (uint32_t)(adc_dma.destinationAddress());
	static const uint32_t dma_memory_midpoint = (uint32_t)(&dma_memory.half_buffers.backhalf);

    //if we're less than halfway through, we should copy over from the back half of memory
	//and if not, copy over from the front half
	if(dma_active_address < dma_memory_midpoint)
		std::copy(  dma_memory.half_buffers.backhalf.begin(), dma_memory.half_buffers.backhalf.end(),
                    block_out.begin());
	else
		std::copy(  dma_memory.half_buffers.fronthalf.begin(), dma_memory.half_buffers.fronthalf.end(),
                    block_out.begin());

    //tweak the ADC readings (12-bit, DC offset) into effectively a Q1.15 datapoint 
    //do this in a C++ style way, hoping for compiler optimizations
    //TODO: DSP SIMD instructions here if we really wanna go the extra mile, not sure if it saves that much compute
    for(int16_t& sample : block_out) { //reference, so we modify our original data
        sample = sample << 4; //use the full 16-bits of the sample (instead of just the 12 from the ADC)
        sample -= 32768; //subtract the nominal DC offset from each sample; don't care too much about accuracy
    }

}
