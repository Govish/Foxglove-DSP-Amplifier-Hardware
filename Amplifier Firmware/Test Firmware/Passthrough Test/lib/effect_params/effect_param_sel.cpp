
#include <effect_param_sel.h>

//just save all the values into the constructor 
Effect_Parameter_Sel::Effect_Parameter_Sel(const std::string _label, App_Span<std::string> _choices, std::string _default_choice):
    Effect_Parameter(_label), //save the label with the parent class
    choices(_choices) //save our span of choices
{
    //no `find()` in C++14, so just doing this manually
    //will technically find the last element of the array matching `_default_choice`
    for(size_t i = 0; i < choices.size(); i++)
        if(choices[i] == _default_choice) choice_index = i;
    
    //function won't modify choice_index if no match is found
    //therefore `choice_index` will default to 0 (first choice)
}

//should basically configure the max value of the encoder and its steps position
//shouldn't attach any callbacks --> that's what the owner program should do
void Effect_Parameter_Sel::attach_configure_enc(Rotary_Encoder& enc) {
    //save the encoder using the parent function
    Effect_Parameter::attach_configure_enc(enc);
    
    //configure the encoder with said number of steps, along with a starting encoder value corresponding to our starting choice
    enc.set_max_counts(choices.size(), choice_index);
}

//read the encoder position, recompute parameter value if the encoder position is different 
void Effect_Parameter_Sel::synchronize() {
    //if we have an attached encoder, read it
    if(enc != nullptr) {
        //choice index directly corresponds to encoder counts
        choice_index = enc->get_counts();
    }
}

//actually get the parameter value
//make sure to call `synchronize()` before reading this
uint32_t Effect_Parameter_Sel::get() { return choice_index; }

//render the parameter
//will show up as a label of the parameter at the bottom
//a bar chart roughly visualizing the value w.r.t. the entire range
//and the actual numerical value above it
void Effect_Parameter_Sel::draw(uint32_t x_offset, uint32_t y_offset, U8G2& graphics_handle) {
    //TODO
}