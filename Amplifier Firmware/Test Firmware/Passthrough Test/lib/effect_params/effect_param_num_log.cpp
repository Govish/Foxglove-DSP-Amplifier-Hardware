
#include <effect_param_num_log.h>

//just save all the values into the constructor 
Effect_Parameter_Num_Log::Effect_Parameter_Num_Log( const std::string _label, const float _param_min, 
                                                    const float _param_max, const uint32_t num_points, const float param_default):
    Effect_Parameter(_label), //save the label with the parent class
    ln_param_min(log(_param_min)), ln_param_max(log(_param_max)), encoder_max_count(num_points)
{
    //compute the starting encoder position given the default value
    last_encoder_count = (uint32_t)map(log(param_default), ln_param_min, ln_param_max, 0, encoder_max_count);

    //compute the parameter value based off this starting value
    //since transforming via log, have to exponentiate after `map()` to get the real parameter value
    //not directly starting at param default to compensate for discretization error
    param_value = exp(map((float)last_encoder_count, 0, encoder_max_count, ln_param_min, ln_param_max));
}

//should basically configure the max value of the encoder and its steps position
//shouldn't attach any callbacks --> that's what the owner program should do
void Effect_Parameter_Num_Log::attach_configure_enc(Rotary_Encoder& enc) {
    //save the encoder using the parent function
    Effect_Parameter::attach_configure_enc(enc);
    
    //configure the encoder with said number of steps, along with a starting encoder value
    enc.set_max_counts(encoder_max_count, last_encoder_count);
}

//read the encoder position, recompute parameter value if the encoder position is different 
void Effect_Parameter_Num_Log::synchronize() {
    //if we have an attached encoder, read it
    if(enc != nullptr) {
        //get the encoder counts
        uint32_t encoder_pos = enc->get_counts();

        //if counts are the same, don't do anything
        if(encoder_pos == last_encoder_count) return;

        //counts are different, recompute parameter value and save the new counts
        //if we take the log, can do this mapping linearly, then exponentiate at the end
        param_value = exp(map((float)encoder_pos, 0, encoder_max_count, ln_param_min, ln_param_max));
        last_encoder_count = encoder_pos;
    }
}

//actually get the parameter value
//make sure to call `synchronize()` before reading this
float Effect_Parameter_Num_Log::get() { return param_value; }

//render the parameter
//will show up as a label of the parameter at the bottom
//a bar chart roughly visualizing the value w.r.t. the entire range
//and the actual numerical value above it
void Effect_Parameter_Num_Log::draw(uint32_t x_offset, uint32_t y_offset, U8G2& graphics_handle) {
    //TODO
}