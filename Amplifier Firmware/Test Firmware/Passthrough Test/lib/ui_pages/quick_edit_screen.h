#pragma once

/*
 * Provide a screen that allows for quick editing of effect parameters
 * Should be constructed by the main page and be initialized with an index corresponding to which encoder/led it's hooked up to
 * Can reconfigure which quick edit parameter is being used and its corresponding theme color
 * Returns to home page after the timeout specified in `App_Constants`
 * 
 * By Ishaan Gov Jan 2023
 */


#include <Arduino.h>

#include <rgb.h>
#include <encoder.h>
#include <scheduler.h> //to schedule page transition
#include <effect_param.h> //parameter interface
#include <ui_page.h> //implements UI page interface
#include <config.h> //app constants

class Quick_Edit_Screen : public UI_Page {

public:
    //have a default constructor and a non-default one
    Quick_Edit_Screen();
    Quick_Edit_Screen(UI_Page* _main_page, const size_t _enc_led_index);

    //if using our default constructor, need to be able to set the main page and the index
    void set_main_pg_index(UI_Page* _main_page, const size_t _enc_led_index);

    //set the particular quick edit parameter of interest
    void set_qe_param_color(Effect_Parameter* _qe_param, RGB_LED::COLOR _theme_color);

private:
    //callback to refresh the quick edit timeout when encoder is changed
    //delays return to main page
    static void refresh_edit_timeout(void* context);

    //override the standard UI page functions
    void impl_on_entry() override;
    void impl_on_exit() override;
    void draw() override;

    //own a page transition back to the main page
    Pg_Transition back_to_main;

    //"index" of the LED and encoder we're going to be using for editing
    //this SHOULD be const, but I need this page to have a default constructor
    //basically I want to make an array of quick edit pages, but it's gonna be really messy to have non-default constructor
    size_t enc_led_index;

    //point to a quick edit parameter of interest and its theme color
    Effect_Parameter* qe_param;
    RGB_LED::COLOR theme_color;

    //own a scheduler that basically calls the transition function once we're done editing
    Scheduler done_editing_sched;
};