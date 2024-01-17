#pragma once

/**
 * Effect that has 5 dummy parameters and just passes audio straight through
 * Configurable Name and theme color
 * Useful for basic testing of the audio path and user interface functionality
 * 
 * By Ishaan Gov Jan 2024
*/

#include <array>
#include <string>

#include <effect_interface.h> //implements interface specified here
#include <effect_edit/default_effect_edit_impl.h> //effect menu implementation
#include <effect_param_num_log.h>   //              ""
#include <dspinst.h> //for DSP and SIMD instruction for speed and such

class Effect_IIR_LP : public Effect_Interface {
public:
    //default constructor -- just call the base class constructor
    Effect_IIR_LP();

    //need to have a non-default copy constructor--> need to freshly instantiate the `effect_edit` param
    //we'll invoke the default constructor using the theme color and name from the `other`
    Effect_IIR_LP(const Effect_IIR_LP& other);
    
    //implement the clone function to produce instances of this effect that are copies of the original
    //need to override base class so we produce copies of the derived class instead
    std::unique_ptr<Effect_Interface> clone() override;

    //provide implementations for the following functions:
    void audio_update(const Audio_Block_t& block_in, Audio_Block_t& block_out) override;
    std::string get_name() override;
    Effect_Icon_t get_icon() override;
    RGB_LED::COLOR get_theme_color() override;
    Effect_Parameter* get_quick_edit_param() override; 

private:
    //define the implementations for the effect edit menu
    //will use the `default_effect_edit_implementation` to handle editing and rendering our parameter menu
    void draw() override;
    void impl_on_entry() override;
    void impl_on_exit() override;
    
    //have an icon for the effect, will be constant for all instances
    static const Effect_Icon_t icon;

    //have a particular name and theme for our instance
    const std::string name; 
    const RGB_LED::COLOR theme_color;    

    //have a parameter that sets the cutoff frequency of the filter
    Effect_Parameter_Num_Log f_cutoff;
    float sync_cutoff_freq = 0;     //synchronized version to recompute IIR constant
    int32_t feedback_factor = 0;    //Q1.31 formatted number that sets the feedback term in the IIR equation
    int32_t feedforward_factor = 0; //Q1.31 formatted number that sets the feed-forward term in the IIR equation
    
    int32_t last_sample = 0;        //single output memory element for our first-order IIR filter

    //helper variables in computing the cutoff frequency
    static constexpr float RADSEC_PER_HZ = TWO_PI;
    static constexpr float EXP_m1 = 0.36787944117;

    //have an instance of our `default_effect_edit_impl`
    //to actually handle our edit menu 
    Default_Effect_Edit_Impl effect_edit;
};  