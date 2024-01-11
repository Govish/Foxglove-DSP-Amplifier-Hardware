
#include <effect_param_sel.h>

//just save all the values into the constructor 
Effect_Parameter_Sel::Effect_Parameter_Sel(const std::string _label, App_Span<std::string> _choices, std::string _default_choice):
    Effect_Parameter(_label), //save the label with the parent class
    active_choice(),  //default initialize the scroll string
    choices(_choices) //save our span of choices
{
    //no `find()` in C++14, so just doing this manually
    //will technically find the last element of the array matching `_default_choice`
    for(size_t i = 0; i < choices.size(); i++)
        if(choices[i] == _default_choice) choice_index = i;
    
    //function won't modify choice_index if no match is found
    //therefore `choice_index` will default to 0 (first choice)

    //configure the scrolling text to scroll at half speed
    active_choice.set_scroll_px_per_update(0.5);
}

//should basically configure the max value of the encoder and its steps position
//shouldn't attach any callbacks --> that's what the owner program should do
void Effect_Parameter_Sel::attach_configure_enc(Rotary_Encoder& enc) {
    //save the encoder using the parent function
    Effect_Parameter::attach_configure_enc(enc);
    
    //configure the encoder with said number of steps, along with a starting encoder value corresponding to our starting choice
    enc.set_max_counts(choices.size() - 1, choice_index);
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
    //NOTE: DON'T CLEAR THE SCREEN BUFFER! WILL BE DONE BY THE HOST PAGE!
    //set the font with which to render all parameter text
    UI_Page::apply_font_small_params();
    u8g2_uint_t font_height = (graphics_handle.getAscent() - graphics_handle.getDescent());
    u8g2_uint_t active_y_bot = y_offset + PARAM_EDIT_RENDER_HEIGHT - font_height - 2;
    u8g2_uint_t active_y_center = (y_offset + active_y_bot)/2;

    //######### Draw vertical lines as a border for our select menu ############
    graphics_handle.drawVLine(x_offset, y_offset, active_y_bot - y_offset);
    graphics_handle.drawVLine(x_offset + PARAM_EDIT_RENDER_WIDTH - 1, y_offset, active_y_bot - y_offset);

    //######### Draw the parameter label at the bottom of the screen ##########
    graphics_handle.setFontPosBottom(); //reference text position from the bottom
    u8g2_uint_t label_width = graphics_handle.getStrWidth(label.c_str());
    graphics_handle.drawStr(x_offset + (PARAM_EDIT_RENDER_WIDTH - label_width)/2, y_offset + PARAM_EDIT_RENDER_HEIGHT, label.c_str());

    //######## Render Choices and the Selected Choice #########
    //this is done in a pretty damn jank way, but its kinda lightweight and easy to implement
    //fine with small number of parameters to select between
    //start by setting our valid graphics area to our active area and our font mode
    graphics_handle.setFontPosCenter();
    graphics_handle.setClipWindow(  x_offset, y_offset, 
                                    x_offset + PARAM_EDIT_RENDER_WIDTH - 2, active_y_bot);

    static const u8g2_uint_t choice_spacing = font_height + 3;
    u8g2_uint_t y_choices = active_y_center - choice_index * choice_spacing; //y position of the first index
    u8g2_uint_t x_choices = x_offset + 2;
    for(size_t i = 0; i < choices.size(); i++) {
        //render the string if it's not our selected choice
        //handle drawing the selected choice slightly differently
        if(i != choice_index)
            graphics_handle.drawStr(x_choices, y_choices, choices[i].c_str());
        
        //increment our y_position by `choice_spacing`
        y_choices += choice_spacing;
    }

    //######### Render the middle item as the selected item ############
    //if we've selected a new item
    if(last_choice_index != choice_index) {
        active_choice.stop(); //stop the scrolling
        active_choice.set_render_text(choices[choice_index]);
        active_choice.start(); //restart the scrolling with the new text
        
        last_choice_index = choice_index; //update choice index
    }
    active_choice.set_bounding_box( y_offset, active_y_bot, 
                                    x_offset + 2, x_offset + PARAM_EDIT_RENDER_WIDTH - 2);
    active_choice.render(graphics_handle);
    
    //######### Render some graphics around the selected item #############
    //TODO: FIX or figure out --> this 2-pixel business is a bit funky
    u8g2_uint_t upper_line_y = active_y_center - (choice_spacing + 2)/2;
    u8g2_uint_t lower_line_y = upper_line_y + choice_spacing;
    
    graphics_handle.setMaxClipWindow(); //restore the graphics to the full screen
    graphics_handle.drawHLine(x_offset, upper_line_y, PARAM_EDIT_RENDER_WIDTH);
    graphics_handle.drawHLine(x_offset, lower_line_y, PARAM_EDIT_RENDER_WIDTH);

    //restore settings back to default
    UI_Page::restore_font_default();
}