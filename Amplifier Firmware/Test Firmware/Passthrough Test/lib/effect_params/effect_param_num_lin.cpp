
#include <effect_param_num_lin.h>

//just save all the values into the constructor 
Effect_Parameter_Num_Lin::Effect_Parameter_Num_Lin( const std::string _label, const float _param_min, 
                                                    const float _param_max, const float _param_step, const float param_default):
    Effect_Parameter(_label), //save the label with the parent class
    param_min(_param_min), param_max(_param_max), 
    encoder_max_count((uint32_t)((param_max - param_min) / _param_step))
{
    //compute the starting encoder position given the default value
    last_encoder_count = (uint32_t)map(param_default, param_min, param_max, 0, encoder_max_count);

    //compute the parameter value based off this starting value
    //not directly starting at param default to compensate for discretization error
    param_value = map((float)last_encoder_count, 0, encoder_max_count, param_min, param_max);
}

//should basically configure the max value of the encoder and its steps position
//shouldn't attach any callbacks --> that's what the owner program should do
void Effect_Parameter_Num_Lin::attach_configure_enc(Rotary_Encoder& enc) {
    //save the encoder using the parent function
    Effect_Parameter::attach_configure_enc(enc);
    
    //configure the encoder with said number of steps, along with a starting encoder value
    enc.set_max_counts(encoder_max_count, last_encoder_count);
}

//read the encoder position, recompute parameter value if the encoder position is different 
void Effect_Parameter_Num_Lin::synchronize() {

    //if we have an attached encoder, read it
    if(enc != nullptr) {
        //get the encoder counts
        uint32_t encoder_pos = enc->get_counts();

        //if counts are the same, don't do anything
        if(encoder_pos == last_encoder_count) return;

        //counts are different, recompute parameter value and save the new counts
        param_value = map((float)encoder_pos, 0, encoder_max_count, param_min, param_max);
        last_encoder_count = encoder_pos;
    }
}

//actually get the parameter value
//make sure to call `synchronize()` before reading this
float Effect_Parameter_Num_Lin::get() { return param_value; }

//render the parameter
//will show up as a label of the parameter at the bottom
//a bar chart roughly visualizing the value w.r.t. the entire range
//and the actual numerical value above it
//x,y offset describe the top left corner of the effect
//by default renders 1 decimal place
void Effect_Parameter_Num_Lin::draw(uint32_t x_offset, uint32_t y_offset, U8G2& graphics_handle) {
    
    //NOTE: DON'T CLEAR THE SCREEN BUFFER! WILL BE DONE BY THE HOST PAGE!
    //set the font with which to render all parameter text
    UI_Page::apply_font_small_params();
    u8g2_uint_t font_height = (graphics_handle.getAscent() - graphics_handle.getDescent());

    //######### Draw parameter value at the top of active screen area ###########
    std::string param_val_raw = std::to_string(param_value);
    std::string param_val_trunc = param_val_raw.substr(0, param_val_raw.find(".")+2);
    
    graphics_handle.setFontPosTop(); //reference text position from the top 
    u8g2_uint_t text_width = graphics_handle.getStrWidth(param_val_trunc.c_str());
    graphics_handle.drawStr(x_offset + (PARAM_EDIT_RENDER_WIDTH - text_width)/2, y_offset, param_val_trunc.c_str());

    //######### Draw the parameter label at the bottom of the screen ##########
    graphics_handle.setFontPosBottom(); //reference text position from the bottom
    u8g2_uint_t label_width = graphics_handle.getStrWidth(label.c_str());
    graphics_handle.drawStr(x_offset + (PARAM_EDIT_RENDER_WIDTH - label_width)/2, y_offset + PARAM_EDIT_RENDER_HEIGHT, label.c_str());

    //######### Draw a frame that represents the min/max value of the parameter ############
    //top of the bar should be right below 
    static const u8g2_uint_t BAR_FRAME_VERTICAL_PADDING = 2;
    static const u8g2_uint_t BAR_FRAME_HORIZONTAL_PADDING = 4;
    u8g2_uint_t bar_frame_top = y_offset + font_height + BAR_FRAME_VERTICAL_PADDING;
    u8g2_uint_t bar_frame_bot = y_offset + PARAM_EDIT_RENDER_HEIGHT - (font_height + BAR_FRAME_VERTICAL_PADDING);
    u8g2_uint_t bar_frame_left = x_offset + BAR_FRAME_HORIZONTAL_PADDING;
    u8g2_uint_t bar_frame_right = x_offset + PARAM_EDIT_RENDER_WIDTH - BAR_FRAME_HORIZONTAL_PADDING;

    graphics_handle.drawFrame(bar_frame_left, bar_frame_top, bar_frame_right - bar_frame_left, bar_frame_bot - bar_frame_top);

    //######## Draw a filled box that represents what the current parameter value is set to ###########
    //bottom of the bar corresponds to the min value
    //top of the bar corresponds to the max value

    static const u8g2_uint_t BAR_PADDING_LR = 2;
    static const u8g2_uint_t BAR_PADDING_TB = 1;

    u8g2_uint_t bar_top = (u8g2_uint_t)map(param_value, param_min, param_max, bar_frame_bot - BAR_PADDING_TB, bar_frame_top + BAR_PADDING_TB);
    graphics_handle.drawBox(bar_frame_left + BAR_PADDING_LR, bar_top, bar_frame_right - bar_frame_left - 2*BAR_PADDING_LR, bar_frame_bot - bar_top);

    //restore the font back to default
    UI_Page::restore_font_default();
}