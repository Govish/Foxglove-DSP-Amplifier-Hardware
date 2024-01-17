#include <audio_level.h>

//======================== STATIC VARIABLE INITIALIZATION =======================

//maintain a list of threshold levels for our visualizer --> initialized during `init()`
std::array<int32_t, App_Constants::LEVEL_VIS_NUM_LEDS> Audio_Level_Vis::high_thresholds = {0};
std::array<int32_t, App_Constants::LEVEL_VIS_NUM_LEDS> Audio_Level_Vis::low_thresholds = {0};

//memory variables for our visualizer
//start these values off at zero
volatile int32_t Audio_Level_Vis::peak_memory = 0; 
volatile int32_t Audio_Level_Vis::peak_decay = 0;

//============================ PUBLIC METHODS ===========================

void Audio_Level_Vis::init() {
    //initialize the pins to output, active low
    for(auto pin : led_pins) {
        pinMode(pin, OUTPUT);
        digitalWrite(pin, !Pindefs::LEVEL_ACTIVE_HIGH);
    }

    //initialize our level thresholds arrays
    //from our configured floating-point THRESHOLD_LEVELS array in our config
    //conversion to fixed point for computation speed
    //have two separate arrays for hysteresis --> makes our LEDs less "flicker-y" near the threshold boundaries
    for(size_t i = 0; i < App_Constants::THRESHOLD_LEVELS.size(); i++) {
        high_thresholds[i] = (int32_t)((App_Constants::THRESHOLD_LEVELS[i] + App_Constants::THRESHOLD_HYSTERESIS/2.0f) * std::numeric_limits<int32_t>::max());
        low_thresholds[i] = (int32_t)((App_Constants::THRESHOLD_LEVELS[i] - App_Constants::THRESHOLD_HYSTERESIS/2.0f) * std::numeric_limits<int32_t>::max());
    }
    

    //compute the uint16_t decay parameter from the desired time constant
    //do this by first computing $e^-1$ --> corresponds to decay after a single time constant
    //from here, figure out the amount of incremental decay for this level of decay to happen after the specified time constant
    //then convert that number into a Q0.32 fixed-point format
    double time_constant_decay_ratio = exp((double)-1.0);
    double tau_decay_samples = (double)App_Constants::LEVEL_VIS_DECAY_TIME_CONSTANT_SEC * (double)App_Constants::AUDIO_SAMPLE_RATE_HZ;
    double decay_per_sample = pow(time_constant_decay_ratio, (double)1.0/tau_decay_samples);
    peak_decay = (int32_t)((double)(std::numeric_limits<int32_t>::max() + 1.0) * decay_per_sample);
}

void Audio_Level_Vis::update(const Audio_Block_t& block_in) {
    //for each sample in the audio block
    for(const auto& sample : block_in) {
        //compute the absolute value of the sample
        //and also bump it up to a 32-bit value
        //do this by shift left 16 (since sample is a signed 16-bit value)
        int32_t sample_magnitude = abs((int32_t)sample) << 16;

        //do the comparison between the sample magnitude and our memory variables
        //save the new magnitude if the sample is greater than our peak
        //acts as an ideal diode charging a peak-detector circuit
        if(sample_magnitude > peak_memory) peak_memory = sample_magnitude;

        //decay our peak memory by a single step
        //`peak_decay` is in a fixed-point decimal Q1.31 format --> 32-bit multiply + 32-bit shift is exactly what we need 
        //use a DSP instruction; may be faster, IDK
        //have to perform an additional left shift since we're only using 31 decimal spaces instead of 32
        //not sure why there isn't an arm instruction like this for unsigned integers but pop off
        peak_memory = multiply_32x32_rshift32(peak_memory, peak_decay) << 1;
    }

    //at the end of our sample process, write the LEDs with the appropriate output state given the peak values
    for(size_t i = 0; i < led_pins.size(); i++) {
        //grab the pin and the thresholds associated with it
        const auto& low_thresh = low_thresholds[i];
        const auto& high_thresh = high_thresholds[i];
        const auto& pin = led_pins[i];

        //do the comparison, write the pin appropriately
        //instantaneous magnitude is stored in `peak_memory`
        //additionally add hysteresis to our LEDs
        if(peak_memory > high_thresh) 
            digitalWriteFast(pin, Pindefs::LEVEL_ACTIVE_HIGH);        
        else if(peak_memory < low_thresh)
            digitalWriteFast(pin, !Pindefs::LEVEL_ACTIVE_HIGH);
    }
}