#include <encoder.h>

//==================================== STATIC VARIABLE INITIALIZATION ==================================

bool Rotary_Encoder::timer_initialized = false; //haven't initialized the timer yet
size_t Rotary_Encoder::num_created_instances = 0; //haven't created any instances yet
std::array<Rotary_Encoder*, App_Constants::NUM_ENCODERS> Rotary_Encoder::ALL_ENCODERS = {nullptr}; //container to hold encoder instances

//====================================== PUBLIC FUNCTIONS ====================================

//Constructor, just initialize all our const variables
Rotary_Encoder::Rotary_Encoder(const uint8_t pin_a, const uint8_t pin_b, const uint8_t pin_sw, const cnt_dir_t cnt_dir):
    a(pin_a), b(pin_b), sw(pin_sw), COUNT_LUT(ENC_LUTs[cnt_dir])
{}

//initialize the input pins
void Rotary_Encoder::init() {
    //set the pins as inputs, enable pullup resistor
    pinMode(a, INPUT_PULLUP);
    pinMode(b, INPUT_PULLUP);
    pinMode(sw, INPUT_PULLUP);

    //initialize the timer if required
    if(!timer_initialized) {
        /*
        * Configure the Periodic Interrupt Timer (PIT)
        *  Timer interrupt frequency is given by:
        *  PIT_clock/(period + 1) --> adapted from formula on p. 3041 NOTE THE `+1`!
        *  We'll be using >> CHANNEL_1 << to trigger the sampling interrupt, run this at the specified frequency
        *  Will likely use other channels for other interrupt and task scheduling around the firmware
        */
        CCM_CSCMR1 |= CCM_CSCMR1_PERCLK_CLK_SEL; //source PIT and GPT clocks from 24MHz oscillator
        CCM_CCGR1 |= CCM_CCGR1_PIT(CCM_CCGR_ON); //enable clock to PIT in all modes
        PIT_MCR = 0; //turn on PIT, enable logic is inverted for whatever reason
        PIT_TCTRL1 = 0; //disable the timer if it's running for whatever reason
        PIT_LDVAL1 = (uint32_t)(24000000.0f * App_Constants::ENC_BOUNCE_TIME) - 1; //set the period of the timer according to config

        //enable interrupt and start the timer
        attachInterruptVector(IRQ_PIT, SAMPLE_ISR);
        NVIC_SET_PRIORITY(IRQ_PIT, App_Constants::ENC_SAMPLING_PRIO);
        NVIC_ENABLE_IRQ(IRQ_PIT);
        PIT_TCTRL1 = PIT_TCTRL_TEN | PIT_TCTRL_TIE;

        //set the flag so we don't do this over
        timer_initialized = true;
    }

    //sample the status of the encoder pins to initialize the encoder history
    //clear any event flags due to bogus values in the history registers
    sample();
    flag_change = false;
    flag_press = false;
    flag_release = false;

    //now that everything is initialized, we can sample and update this particular channel
    if(num_created_instances > App_Constants::NUM_ENCODERS - 1) while(1); //TODO gracefully error
    ALL_ENCODERS[num_created_instances] = this; //store a pointer to the created instance
    num_created_instances++; 
}

//getter and setter methods of switch counts and states
bool Rotary_Encoder::get_switch() { return current_switch; }
int32_t Rotary_Encoder::get_counts() { return encoder_count; }


//NOTE: this function isn't interrupt safe!
//i.e. if function is interrupt between update of max counts and `set_counts()`
//last counts may have an out of range encoder value, but encoder_counts would be constrained
//thus such an event would trigger a change interrupt
//as such, ensure no interrupts fire  between max counts updating and count updating
void Rotary_Encoder::set_max_counts(int32_t _max_counts, int32_t reset_val) {
    if(_max_counts < 0) _max_counts = 0; //sanity check

    //========== ATOMIC SECTION ==========
    NVIC_DISABLE_IRQ(IRQ_PIT);
    encoder_max_count = _max_counts; //update the new max count value
    set_counts(reset_val); //reset the encoder counts to the desired value
    NVIC_ENABLE_IRQ(IRQ_PIT);
    //========= end ATOMIC SECTION ========
}

//NOTE: I think this function is ISR safe; the int32_t write should be atomic
//and change in counts shouldn't affect change detection algorithm
void Rotary_Encoder::set_counts(int32_t counts) {
    //set constrain the valid range
    if(counts < 0) counts = 0;
    else if (counts > encoder_max_count) counts = encoder_max_count;

    //actually update the encoder counts
    encoder_count = counts;
}

//attaching callback functions --> pretty straightforward, just copy over
void Rotary_Encoder::attach_on_change(Context_Callback_Function<void> _on_change) { on_change = _on_change; }
void Rotary_Encoder::attach_on_press(Context_Callback_Function<void> _on_press) { on_press = _on_press; }
void Rotary_Encoder::attach_on_release(Context_Callback_Function<void> _on_release) { on_release = _on_release; }

//update --> call necessary callback functions from this context
void Rotary_Encoder::update() {
    //execute callbacks and clear flags as necessary
    //clear flags first if callback functions take a while for whatever reason
    if(flag_press) {
        flag_press = false;
        on_press();
    }

    if(flag_release) {
        flag_release = false;
        on_release();
    }

    if(flag_change) {
        flag_change = false;
        on_change();
    }
}

//convenience function that automatically calls `update_all()` on all instances
void Rotary_Encoder::update_all() {
    for(Rotary_Encoder* enc : ALL_ENCODERS)
        if(enc != nullptr) enc->update();
}

//=============================== PRIVATE MEMBER FUNCTIONS ==============================

void Rotary_Encoder::sample() {
    //only read the switch every handful of samples!
    if(switch_sample_counter) switch_sample_counter--;
    else {
        //update the switch history, read the current switch, and set the flag press/release flags as necessary
        last_switch = current_switch;
        current_switch = digitalReadFast(sw);
        if(!last_switch && current_switch) flag_release = true; //inverted logic; low to high transition
        if(last_switch && !current_switch) flag_press = true; //inverted logic, high to low transition

        //also reset the switch sample interval counter
        switch_sample_counter = switch_sample_interval - 1;
    }
    
    //update the encoder history, read the encoder pins, inc/dec the count according to LUT, set flags as necessary
    last_encoder_count = encoder_count;
    encoder_history = (encoder_history << 1) | (digitalReadFast(a) ? 0 : 1); // shift over, read in A
    encoder_history = (encoder_history << 1) | (digitalReadFast(b) ? 0 : 1); // shift over again, read in B
    int32_t enc_delta = (int32_t)(COUNT_LUT[encoder_history & ENC_HISTORY_LUT_MASK]); //read how much we need to change encoder counts by
    
    //conditionally increment or decrement to respect encoder limits ( [0, max_count] )
    if(enc_delta > 0) encoder_count += (encoder_count == encoder_max_count) ? 0 : enc_delta;
    else if(enc_delta < 0) encoder_count += (encoder_count == 0) ? 0 : enc_delta;

    //set the flag if encoder count changed, set the change flag
    if(encoder_count != last_encoder_count) flag_change = true;
}

//sample all encoders on timer interrupt
void Rotary_Encoder::SAMPLE_ISR() {
    //clear the PIT interrupt flag
    PIT_TFLG1 = 1;

    for(Rotary_Encoder* enc : ALL_ENCODERS)
        if(enc != nullptr) enc->sample();
}