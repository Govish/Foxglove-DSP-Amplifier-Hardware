#pragma once

/*
 * File that basically describes the interface of all audio effects
 * As such, all effects must implement this interface and override all functions as necessary
 * 
 * Functions to implement:
 *  - connect()
 *      >>> gets called when the effect is added to the chain
 *      - initialize operational variables and constants
 *      - set up events and timing
 *  - disconnect()
 *      >>> gets called when the effect is removed from the chain
 *      - reset operational variables and constants
 *      - deschedule events, reset timing, etc. 
 *  - audio_update(const std::array& block_in, std::array& block_out)
 *      >>> gets called in ISR context; this is where the effect is implemented
 *      - synchronize all the parameters
 *      - get the parameters (read into local variables as necessary); act on the parameter changes
 *      - run through the audio block and apply the effect
 *  
 *  - std::string get_name() 
 *      >>> return the name of the effect
 *      
 *  
 *  - Effect_Icon_t get_icon()
 *      >>> return the graphic icon for the pedal to be rendered on the home screen
 *      - I can't enforce (in a reconfigurable way) that an icon member variable exists
 *              but this is the next best thing
 *      - return an icon as an XBM-style array for the effect home screen artwork
 * 
 * Each effect interface will also inherit from the effect edit screen 
 *  - point the `display_name` to a string containing the display name
 *      - can point this to `name` or some different string
 *  - set the color to the theme color of the effect
 *  - point the parameters to parameter instances as necessary
 *  - Can override `on_entry()`, `on_exit()` and `draw()` as necessary
 */

#include <array>
#include <string>
#include <memory> //unique_ptr
#include <Arduino.h>

#include <ui_page.h> //effect interface is an effect edit page
#include <effect_param.h> //for quick edit parameters
#include <rgb.h> //for colors
#include <config.h> //for audio block size

//defining this icon type outside of the class
typedef std::array<uint8_t, (App_Constants::EFFECT_ICON_WIDTH + 7)/8 * App_Constants::EFFECT_ICON_HEIGHT> Effect_Icon_t;

class Effect_Interface : public UI_Page {
public:
    Effect_Interface(): to_return_page() {} //default constructor just initializes the page transition

    //need to declare the base desctructor as virtual
    //this ensures that calls to `delete Effect_Interface*` get redirected to derived destructors
    //extra important since if we don't do this, `std::unique_prt<Effect_Interface>` will only call the base destructor
    //https://stackoverflow.com/questions/461203/when-to-use-virtual-destructors
    virtual ~Effect_Interface() {}

    //have a function that sets the return page after editing is complete
    //just updates the page transition that brings it from the edit page to the return page
    //HOWEVER, children are responsible for invoking the transition
    void set_return_page(UI_Page* _return_page) { to_return_page.set_to(_return_page); }

    //add the particular effect into the effect chain
    //implemented by children
    virtual void connect() {}

    //remove the particular effect from the effect chain
    //implemented by children
    virtual void disconnect() {}

    //heap-allocate a copy of an instance
    //and return a `unique_ptr<Effect_Parameter> that dereferences to that heap location
    //need to implement in derived classes since we need the derived copy constructor
    //leveraging copy elision to make this function happen (since `std::unique_ptr` has deleted copy constructor)
    //implemented by children; default implementation creates a base class instance
    virtual std::unique_ptr<Effect_Interface> clone() { return std::make_unique<Effect_Interface>(*new Effect_Interface(*this)); }

    //CORE OF THE EFFECT: actually run the effect with audio data
    //don't modify the input buffer, but can modify the output buffer
    virtual void audio_update(const Audio_Block_t& block_in, Audio_Block_t& block_out) {}

    //all effects must be able to return their name (to select them from a menu)
    virtual std::string get_name() { return "Default Name"; } //return some generic string as a name

    //all effects must be able to return an icon (of the specified dimensions) to render them on the home screen
    virtual Effect_Icon_t get_icon() { return {0}; } //return an empty icon by default

    //all effects must be able to return a theme color for lighting LEDs on the home screen
    virtual RGB_LED::COLOR get_theme_color() { return RGB_LED::PURPLE; } //return purple by default

    //all effects must also be able to report a quick edit parameter for home screen adjustment
    //nullptr is a valid option if no quick edit parameters are to be available
    virtual Effect_Parameter* get_quick_edit_param() { return nullptr; } //return nullptr by default

protected:
    //override entry, exit, and draw functions from the `UI_Page()` class
    //these are called when the effect edit menu is invoked
    //allow overriding by children too
    void impl_on_entry() override {}
    void impl_on_exit() override {}
    void draw() override {}

    //own a page transition that brings us back to the return screen
    //all effects will have one, and they'll be responsible for invoking it appropriately
    Pg_Transition to_return_page;
};