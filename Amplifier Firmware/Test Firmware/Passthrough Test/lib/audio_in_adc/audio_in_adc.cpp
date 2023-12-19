#include "audio_in_adc.hpp"

#include "imxrt.h" //for register-level control

void Audio_In_ADC::init() {
    //configure periodic interval timer to fire a trigger at the audio sample rate
    //this is different from the quad timer from the audio library--GPT runs at lower frequency
    //can operate independently of CPU/Bus frequencies (and is thus more predictable)

    /*
     * Configure the Interval Timer
     * 
     */

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
		ADC_ETC_TRIG_CHAIN_CSEL0(App_Constants::INPUT_ADC_CHANNEL) | ADC_ETC_TRIG_CHAIN_B2B0; //no need to add any measurement delays

    /*
     * Perform ADC configuration at the register level
     *  We have a pretty specialized application, and getting the ADCs to do what we want
     *  Using existing library methods might be more effort than it's worth
     * 
     *  We'll instead configure the ADC (ADC2) at the register level, similar to how it's done
     *  in the audio library
     * 
     * NOTE: total ADC conversion time set bY:
     *  t_conv = sfc_adder + average_num * (bct + lst_adder)
     *      single/first continuous time adder --> 4 ADCK cycles + 2 bus clock cycles
     *      average number factor --> 1 - 32x
     *      base conversion time --> 25 ADCK cycles (12-bit)
     *      long sample time adder --> 3 - 25 ADCK cycles
     * 
     */

    //set our averaging, conversion trigger
    //  conversion speed, conversion mode, sample time,
    //  clock division, and clock source
    uint32_t adc_config_reg = 0;
    adc_config_reg |= ADC_CFG_AVGS(2); //16x oversampling
    adc_config_reg |= ADC_CFG_MODE(2); //12-bit conversions
    adc_config_reg |= ADC_CFG_ADTRG; //hardware triggers
    adc_config_reg |= ADC_CFG_ADICLK(3); //use asynchronous ADC clock
    adc_config_reg |= ADC_CFG_ADIV(0); //TODO: set clock divide
    adc_config_reg |= ADC_CFG_ADHSC; //TODO set high speed conversion?
    adc_config_reg |= ADC_CFG_ADSTS(0); //TODO: configure sample time
    adc_config_reg |= ADC_CFG_ADLSMP; //TODO: set long sample time?
    ADC2_CFG = adc_config_reg;

    //set our conversion trigger source from the ADC external trigger controller (adc_etc)
    //don't need to set the conversion complete interrupt, since it'll be serviced by DMA
    ADC2_HC0 = ADC_HC_ADCH(16); //ADC_ETC

    //enable general control parameters --> hardware averaging, internal clock, DMA?
    ADC2_GC = ADC_GC_AVGE | ADC_GC_ADACKEN /*| ADC_GC_DMAEN */; //TODO: DMA?

    //calibrate ADCs
    ADC2_GC |= ADC_GC_CAL;
    while(ADC2_GC & ADC_GC_CAL); //wait for calibration to complete
    if(ADC2_GS & ADC_GS_CALF) while(1); //hang if calibration failed TODO: fail gracefully

    /*
     * Configure our DMA
     * 
     */
}

void Audio_In_ADC::start() {

}