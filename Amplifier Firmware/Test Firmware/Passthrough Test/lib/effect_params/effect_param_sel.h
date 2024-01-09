#pragma once

/* 
 * Parameter that allows for the selection from a list of values
 * Emulates a rotary switch that selects between a couple different values
 * I can't really return an enum directly from the parameter
 *      so this should be treated as indexing into an array of enums that have already been defined
 * 
 * Will leverage the DIY implementation of `std::span` to reference an arbitrarily-sized string container of labels
 */

#include <string>

#include <effect_param.h>
#include <encoder.h>
#include <utils.h>

class Effect_Parameter_Sel : public Effect_Parameter {
public:
    //constructor, takes in labels of all the choices
    //single encoder count per choice
    //if default choice isn't in the list of choices, it'll default choice will be the first element
    Effect_Parameter_Sel(const std::string _label, App_Span<std::string> _choices, std::string _default_choice);

    //should basically configure the max value of the encoder and its steps position
    //shouldn't attach any callbacks --> that's what the owner program should do
    //also save this encoder for when we read our parameter
    void attach_configure_enc(Rotary_Encoder& enc) override;

    //render the parameter
    //will basically show up as a mini menu with the different choices as options
    void draw(uint32_t x_offset, uint32_t y_offset, U8G2& graphics_handle) override;

    //synchronize will latch the actual choice value 
    //and save it to a member variable; parameter value can be retrieved with `get`
    void synchronize() override;
    
    //actually get the parameter value
    //this is an index into the string list provided to the function
    //`Effect` has to act on this numerical value accordingly
    uint32_t get();

private:
    //point to an array of strings that name each of the choices
    App_Span<std::string> choices;

    //save the previous encoder count to remember which choice we selected
    uint32_t choice_index = 0; //default to first choice 
};