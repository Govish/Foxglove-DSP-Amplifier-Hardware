#pragma once

/*
 * Default implementation of an edit page for an effect
 * An effect that implements the default version of this implement page must configure these variables:
 *      \--> Array of pointers to effect parameters --> these are the parameters that will be actively adjusted by knobs and such
 *                                                      can technically be reconfigured dynamically, just call `on_entry()` once this changes
 *                                                      index in array corresponds to which encoder it will be attached to
 *      \--> Theme color --> color of the RGB LEDs when editing effect parameters
 *      \--> pointer to a string `display_name` --> text written at the top of the edit page; can be reconfigured dynamically, just call `draw()`
 * 
 * `on_entry()` will for each non-null parameter
 *      \--> attach the encoder to the particular parameter
 *      \--> light up its corresponding LED at its `dim` level (with the theme color)
 *      \--> attach encoder `on-change()` callback functions to set the LEDs to their bright value for 100ms (with scheduler)
 *          \--> also attach a callback to `draw()` either at fixed frame rate or on parameter change
 *      \--> attach encoder `on-press()` callback functions to save a quick edit parameter and return to the home page
 * 
 * `on_exit()` will for each non-null parameter
 *      \--> detach the encoder from the particular parameter
 *      \--> turn the corresponding LED off
 * 
 * `draw()` will for each non-null parameter
 *      \--> draw the name of the effect on as a header, centered
 *      \--> draw a graphic line right below it
 *      \--> call `draw()` on each of the parameters (if non-null)
 *          \--> stepping over the width of the parameter + 2px
 *          \--> adding 2px vertical spacing too
 * 
 */


#include <array>
#include <string>
#include <Arduino.h> //types, interface
#include <U8g2lib.h> //for display handle

#include <ui_page.h> //inherits from `UI_Page`
#include <effect_param.h> //effect parameter interface
#include <encoder.h> //for rotary encoder reading and event scheduling
#include <rgb.h> //for LED control during splash screen
#include <scheduler.h> //for scheduling LED flashing
#include <config.h> //num LEDs, LED colors, splash screen timing defaults

class Default_Effect_Edit_Impl {
public:
    //attach a function that gets called when we want to transition out of this page (normally a `Pg_Transition`)
    //also pass in the LEDs and encoder array owned by the UI class
    Default_Effect_Edit_Impl(   Context_Callback_Function<void> _transition_cb,
                                std::array<RGB_LED*, App_Constants::NUM_RGB_LEDs>& _leds, 
                                std::array<Rotary_Encoder*, App_Constants::NUM_ENCODERS>& _encs);

    //delete the copy constructor --> always need to instantiate something fresh
    //this is to ensure all the PRC's (see below) point to the correct instances upon copy 
    Default_Effect_Edit_Impl(const Default_Effect_Edit_Impl& other) = delete;

    //call this function to render the edit screen
    //pass in a graphics handle that will actually do the rendering
    void render(U8G2& graphics_handle);

    //set up all LEDs and encoders for rendering
    //will be called automatically by `render` as necessary, so may not really need to use this
    //however, adding this here just to ensure 
    void configure_render_resources();

    //release any resources used to render the edit page
    //should call this function on page exit
    void release_render_resources();

    //call these functions to set some of the rendering details for the effect
    //NOTE: for `set_render_parameter()` can pass `nullptr` to get rid of param at that index
    void set_render_parmeter(Effect_Parameter* param, size_t index);
    void set_display_text(const std::string _display_text);
    void set_LED_color(RGB_LED::COLOR _theme_color);

    //provide a function to retrieve the quick edit parameter
    //likely useful for `get_quick_edit_param()` function in the effect interface
    Effect_Parameter* get_quick_edit_param();

private: 
    //create a struct that allows us to access an effect page and the index of the particular effect channel
    //need this if we want to access a specific LED, encoder, or parameter specific to this effect instance
    //may sometimes be able to get away with just a pointer to the specific object we wanna operate on
    //  but other times, need access to instance information and another object (and thus need pointers to both)
    struct Param_Resource_Collection {
        Default_Effect_Edit_Impl* instance;
        Effect_Parameter* param = nullptr;
        Rotary_Encoder* enc;
        RGB_LED* led;
        Scheduler led_sched;
        bool configured = false;

        //and a function to make an array of these
        //takes pointer to the current instance
        //and arrays of LED and encoder pointers to assign element-wise
        template<size_t N>
        static constexpr std::array<Param_Resource_Collection, N> mk_prc(
            Default_Effect_Edit_Impl* inst,
            std::array<Rotary_Encoder*, N>& encs,
            std::array<RGB_LED*, N>& leds)
        {
            std::array<Param_Resource_Collection, N> a{};
            for (size_t i = 0; i < N; i++) {
                Param_Resource_Collection& x = a[i];
                x.instance = inst;
                x.enc = encs[i];
                x.led = leds[i];
            }
            return a;
        }

        //another utility function to convert this into a void* to make `Callback`s easy
        inline operator void *() {
            return reinterpret_cast<void*>(this);
        }   
    };
    
    //these functions will be called to attach or detach resources to the parameter
    //index specifies position in the parameter array for which to operate
    void configure_parameter(Param_Resource_Collection& prc);
    void release_parameter(Param_Resource_Collection& prc);


    //function to set the quick edit parameter and exit (as a callback)
    static void set_quick_edit_leave_cb(void* context);

    //function to dim the LED back down to the theme color (as a callback)
    static void set_led_dim_cb(void* context);

    //function to brighten the LED (as a callback)
    static inline void set_led_bright_schedule_draw_cb(void* context);

    //own an array of Page and Parameter indices --> will pass one of these as context to callback functions
    //we need an object to initialize this array so will make populate this in the constructor
    //this really should be const, but `reinterpret_cast()` gets unhappy when it is 
    std::array<Param_Resource_Collection, App_Constants::NUM_EDIT_PARAMS> params_and_resources;

    //call this function when we want to transition out of the edit page
    Context_Callback_Function<void> transition_cb;

    //need a pointer to which parameter we're is gonna be our quick-edit parameter
    //this gets set depending on which encoder gets clicked
    Effect_Parameter* quick_edit = nullptr;

    //store some text that we'll render as a header on the screen
    //could theoretically edit this on the fly
    std::string display_text = "";

    //save a color that corresponds to the effect's color theme (this is the color we'll light our LEDs with)
    RGB_LED::COLOR theme_color;
};