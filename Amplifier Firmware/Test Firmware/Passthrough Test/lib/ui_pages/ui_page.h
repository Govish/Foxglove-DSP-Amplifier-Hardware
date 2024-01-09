#pragma once

/*
 * Define the basic interface for a page in the UI system
 * Intention is to provide some common function hooks for other classes to leverage
 * While also having page-specific implementations
 * 
 * NOTE:    virtual functions on embedded systems typically come with some degree of performance tradeoff
 *          However, running the UI system isn't particularly speed-critical for this application
 *          And we might get lucky with some degree of compiler optimization
 */

#include <Arduino.h> //for types, interface
#include <U8g2lib.h> //for graphics handle

#include <encoder.h> //for rotary encoder type
#include <rgb.h> //for RGB LED type

#include <utils.h> //callback function for transition class
#include <scheduler.h> //to call the draw function
#include <config.h>

class UI_Page {
public:
    //do nothing in the constructor
    UI_Page() {}

    //define destructor as virtual
    //ensures that derived destructor is always called
    virtual ~UI_Page() {}

    //function that gets called when this page is loaded
    //anticipate calling this when going from a different page to this one (will be called automatically by `Pg_Transition` class)
    void on_entry();

    //function that gets called when we're leaving this page
    //anticipate calling this when going from this page to a different one (will be called automatically by `Pg_Transition` class)
    void on_exit();

    //return the `UI_Page` that is currently active
    //useful for page transitions
    static inline UI_Page* get_active_page() { return active_page; }

//children can use the graphics handle too
protected:
    //function called to render the particular UI page
    //will be using the U8G2 library to do all the graphics on the OLED; invoke any draw methods using the graphics handle from constructor
    //intended to be overridden by any child pages (declaring as virtual so child function will always be called)
    virtual void draw() {}

    //children can override these implementation-specific `on_entry()` and `on_exit()` functions
    virtual void impl_on_entry() {}
    virtual void impl_on_exit() {}
    

    //static callback function to forward to the instance `draw()` function
    static inline void draw_cb(void* context) { reinterpret_cast<UI_Page*>(context)->draw(); }

    //graphics handle, LEDs, and rotary encoder instances will be shared across all pages
    //will be initialized before any UI code executes
    static U8G2& graphics_handle;
    static std::array<RGB_LED*, App_Constants::NUM_RGB_LEDs>& leds;
    static std::array<Rotary_Encoder*, App_Constants::NUM_ENCODERS>& encs;

private:
    //statically own a pointer to an instance saying what is the active UI page
    //useful when invoking page transitions --> always transition out of the active page
    static UI_Page* active_page;

    //own a scheduler that calls `draw()` at the application configured frame rate
    //only the base class should be able to touch this
    static Scheduler draw_sched;
};

/* 
 * Quick utility class to make transitions between two UI pages convenient and explicit
 * Kinda experimenting with what I can do with C++ here, so hopefully all is working as expected here
 * Set up a from-->to style page transition, execute the transition by just invoking the instance
 * Class overrides the call operator `()` and will execute each page's `on_exit()` and `on_entry()` methods
 * 
 * NOTE: not confident enough in my C++ abilities to say how these will behave when `Pg_transitions` are stack allocated
 * Out of safety, always declare `Pg_transition`s as static, i.e. ensure global allocation 
 * 
 * By Ishaan Gov December 2023
 */

class Pg_Transition {
public:
    //create a transition
    Pg_Transition(): pg_to(nullptr) {}
    Pg_Transition(UI_Page* _pg_to): pg_to(_pg_to) {}
    
    //update source or destination pages
    inline void set_to(UI_Page* _pg_to) { pg_to = _pg_to; }
    
    //invoke the transition; just call the `on_exit()` and `on_entry()` methods appropriately
    inline void operator()() {
        //don't allow calling functions on pages that don't exist
        if(pg_to == nullptr) return;
        UI_Page::get_active_page()->on_exit(); 
        pg_to->on_entry(); 
    }

    //automatically create a callback function when a conversion to a callback is attempted
    //exclusively define this for <void*> type `Context_Callback_Function`s for now
    inline operator Context_Callback_Function<void> () {
        return Context_Callback_Function<void>(reinterpret_cast<void*>(this), forward);
    }

    //callback forwarding function, effectively call `this()` 
    static inline void forward(void* context) {
        reinterpret_cast<Pg_Transition*>(context)->operator()();
    }

private:
    //hold pointers to the pages we want to transition between
    UI_Page* pg_to;
};