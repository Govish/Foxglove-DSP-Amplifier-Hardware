
#include <effect_edit/default_effect_edit_impl.h>

//splash screen takes the graphics handle and some LEDs to animate
//also pass in the page to render after splash screen is complete
Default_Effect_Edit_Impl::Default_Effect_Edit_Impl( Context_Callback_Function<void> _transition_cb,
                                                    std::array<RGB_LED*, App_Constants::NUM_RGB_LEDs>& _leds, 
                                                    std::array<Rotary_Encoder*, App_Constants::NUM_ENCODERS>& _encs):
    //initialize our array of params and corresponding resources
    params_and_resources(Param_Resource_Collection::mk_prc(
        this,   //point the instance we're creating
        _encs,  //array of encoders, assign sequentially
        _leds   //array of LEDs, assign sequentially
    )),
    transition_cb(_transition_cb) //call this function when we want to transition out of the edit page
{}

//this function does nothing --> do parameter configuration in `render()` as necessary
//an implementation of this sort lets us dynamically add/remove parameters from the screen
//and only does so when actively rendering
void Default_Effect_Edit_Impl::configure_render_resources() {}

//this function basically releases each parameter
//need to do this on exit so our parameters don't errantly read from encoders 
void Default_Effect_Edit_Impl::release_render_resources() {
    //release each parameter
    for(Param_Resource_Collection& prc : params_and_resources)
        release_parameter(prc);
}

/*
 * `draw()` will for each non-null parameter
 *      \--> draw the name of the effect on as a header, centered
 *      \--> draw a graphic line right below it
 *      \--> call `draw()` on each of the parameters (if non-null)
 *          \--> stepping over the width of the parameter + 2px
 *          \--> adding 2px vertical spacing too
 */
void Default_Effect_Edit_Impl::render(U8G2& graphics_handle) {
    //configure parameters and resources as necessary
    for(Param_Resource_Collection& prc : params_and_resources)
        configure_parameter(prc);   

    
    //compute the total width of all the active effects
    //start with a negative value here since we're gonna always be adding the `render_padding` for each effect
    int32_t all_param_render_width = -App_Constants::PARAMETER_RENDER_PADDING;
    for(Param_Resource_Collection& prc : params_and_resources) {
        if(prc.param == nullptr) continue; //not gonna render nullptr params, so just continue
        all_param_render_width += prc.param->get_edit_width() + App_Constants::PARAMETER_RENDER_PADDING;
    }

    //##### actual rendering happens now! #####
    //start by clearing the display buffer
    graphics_handle.clearBuffer();

    //############## HEADER AND UNDERBAR ##############

    //draw the header text at the top of the page; compute some constants for doing so
    u8g2_uint_t text_height = graphics_handle.getAscent() - graphics_handle.getDescent();    
    static const u8g2_uint_t HEADER_TEXT_X = (graphics_handle.getDisplayWidth() - graphics_handle.getStrWidth(display_text.c_str())) >> 1;
    static const u8g2_uint_t HEADER_TEXT_Y = 0;

    graphics_handle.setFontPosTop(); //use the top of the font as a handle for easy placement
    graphics_handle.drawStr(HEADER_TEXT_X, HEADER_TEXT_Y, display_text.c_str());
    graphics_handle.setFontPosBaseline(); //restore to default

    //draw a horizontal bar underneath the header
    u8g2_uint_t UNDERBAR_Y = text_height + 1;
    graphics_handle.drawHLine(0, UNDERBAR_Y, graphics_handle.getDisplayWidth());

    //################## RENDER PARAMS ##################
    //render params such that they're horizontally and vertically centered in the space below the header
    u8g2_uint_t param_x_pos = (graphics_handle.getDisplayWidth() - (u8g2_uint_t)all_param_render_width) >> 1;

    for(Param_Resource_Collection& prc : params_and_resources) {
        if(prc.param == nullptr) continue; //don't render nullptr params

        //if the param exists, compute the y-position it should be rendered at
        u8g2_uint_t param_y_pos = ((graphics_handle.getDisplayHeight() - (UNDERBAR_Y + 2) - // valid region height
                                    prc.param->get_edit_height() + 1) >> 1 )                // height of param --> vertical offset from top
                                    + (UNDERBAR_Y + 2);                                     // and offset this from the starting point of the valid region
        
        //render the parameter at the specified position
        prc.param->draw(param_x_pos, param_y_pos, graphics_handle);

        //move the x-position over by the parameter width and whatever padding 
        param_x_pos += prc.param->get_edit_width() + App_Constants::PARAMETER_RENDER_PADDING;
    }

    //############# end RENDERING #############

    //send the display buffer
    graphics_handle.sendBuffer();

}


//call these functions to set some of the rendering details for the effect
//NOTE: for `set_render_parameter()` can pass `nullptr` to get rid of param at that index
void Default_Effect_Edit_Impl::set_render_parmeter(Effect_Parameter* param, size_t index) {
    //sanity check our index; return if index invalid
    if(index >= params_and_resources.size()) return;
    
    //get the existing parameter and its resources at the particular index and release them
    //  then replace the parameter with the new parameter; DON'T initialize in case we're not rendering
    //      instead let the `render()` function take care of parameter configuration
    Param_Resource_Collection& prc = params_and_resources[index];
    release_parameter(prc); //resets `configured` flag automatically
    prc.param = param; 
}

//simple setter methods for the render text
void Default_Effect_Edit_Impl::set_display_text(const std::string _display_text) { display_text = _display_text; }

//set a new theme color
//need to release all parameters and reconfigure them given the new theme color
void Default_Effect_Edit_Impl::set_LED_color(RGB_LED::COLOR _theme_color) {
    theme_color = _theme_color;
    for(Param_Resource_Collection& prc : params_and_resources)
        release_parameter(prc);
}

//quick function to get a quick edit parameter
Effect_Parameter* Default_Effect_Edit_Impl::get_quick_edit_param() { return quick_edit; }

//================================= PRIVATE (CALLBACK) FUNCTION DEFS ==============================

void Default_Effect_Edit_Impl::configure_parameter(Param_Resource_Collection& prc) {
    //don't run through this process more than once
    if(prc.configured) return;
    
    //just set our `configured` flag up front out of ease
    prc.configured = true;
    
    //don't operate on nullptr parameters
    if(prc.param == nullptr) return;

    //attach the encoder to the particular parameter
    prc.param->attach_configure_enc(*prc.enc);

    //light up the particular LED to its dim level
    prc.led->set_color(theme_color);
    prc.led->set_brightness(App_Constants::UI_LED_LEVEL_DIM);

    //attach on change to light the LED for a little bit,
    //attaching pointer to particular prc instance as context --> this is OKAY since statically allocated
    prc.enc->attach_on_change(  Context_Callback_Function<void>(prc, set_led_bright_schedule_draw_cb));

    //attach on press to save quick-edit parameter and go back to the home page
    //attaching pointer to particular prc instance as context --> this is OKAY since statically allocated
    prc.enc->attach_on_press(Context_Callback_Function<void>(prc, set_quick_edit_leave_cb));
}

void Default_Effect_Edit_Impl::release_parameter(Param_Resource_Collection& prc) {
    //don't run through this process more than once
    if(!prc.configured) return;

    //set our configured flag to be `false` up front, out of ease
    prc.configured = false;

    //don't operate on nullptr parameters
    if(prc.param == nullptr) return;

    //detach the encoder to the particular parameter and remove the callback functions
    prc.param->detach_enc();
    prc.enc->attach_on_change({});
    prc.enc->attach_on_press({});

    //turn the LED off
    prc.led->set_color(RGB_LED::OFF);
}

//function to set the quick edit parameter and exit (as a callback)
void Default_Effect_Edit_Impl::set_quick_edit_leave_cb(void* context) {
    //get the particular param + resource collection
    Param_Resource_Collection& prc = *reinterpret_cast<Param_Resource_Collection*>(context); 

    //set the quick-edit field of the instance corresponding to the param + resource collection
    prc.instance->quick_edit = prc.param;

    //now, just run the transition callback function corresponding to the implementation instance
    prc.instance->transition_cb();
}

//function to dim the LED back down to the theme color (as a callback)
void Default_Effect_Edit_Impl::set_led_dim_cb(void* context) {
    //get the particular param + resource collection
    Param_Resource_Collection& prc = *reinterpret_cast<Param_Resource_Collection*>(context); 

    //dim the LED
    prc.led->set_brightness(App_Constants::UI_LED_LEVEL_DIM);
}

//function to brighten the LED (as a callback)
void Default_Effect_Edit_Impl::set_led_bright_schedule_draw_cb(void* context) {//forwarding function
    //get the particular param + resource collection
    Param_Resource_Collection& prc = *reinterpret_cast<Param_Resource_Collection*>(context); 

    //brighten the corresponding LED
    prc.led->set_brightness(App_Constants::UI_LED_LEVEL_BRIGHT);

    //schedule dimming in a little bit 
    prc.led_sched.schedule_oneshot_ms(  Context_Callback_Function<void>(context, set_led_dim_cb),
                                        App_Constants::UI_LED_CHANGE_BRIGHT_TIME_MS);
}