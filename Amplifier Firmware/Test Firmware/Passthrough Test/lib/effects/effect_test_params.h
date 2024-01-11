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
#include <effect_param_sel.h>       //this and below are types of parameters
#include <effect_param_num_lin.h>   //              ""
#include <effect_param_num_log.h>   //              ""
#include <utils.h> //for app span

class Effect_Test_Param : public Effect_Interface {
public:
    //default constructor -- just call the base class constructor
    Effect_Test_Param(RGB_LED::COLOR _theme_color, std::string _name);

    //need to have a non-default copy constructor--> need to freshly instantiate the `effect_edit` param
    //we'll invoke the default constructor using the theme color and name from the `other`
    Effect_Test_Param(const Effect_Test_Param& other);
    
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

    //have a particular name for our instance
    const std::string name; 

    //also have a theme color that can be configured instance-by-instance
    const RGB_LED::COLOR theme_color;    

    //some random choices for the `sel_param`
    //declare this before `sel_param` to initialize this before the particular member!
    std::array<std::string, 5> choices = {
        "Type 1",
        "Type 2",
        "Type 3", 
        "Type 4",
        "Type 5"
    };

    //hold some parameters for testing
    Effect_Parameter_Num_Lin lin_param;
    Effect_Parameter_Num_Log log_param;
    Effect_Parameter_Sel sel_param;

    //have an instance of our `default_effect_edit_impl`
    //to actually handle our edit menu 
    Default_Effect_Edit_Impl effect_edit;
};  