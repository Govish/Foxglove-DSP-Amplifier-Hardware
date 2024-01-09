#pragma once

#include <array>
#include <Arduino.h> //types, interface
#include <U8g2lib.h> //for display handle

#include <ui_page.h> //inherits from `UI_Page`
#include <blank_dwell_screen.h> //to dwell on a blank screen before running main app>
#include <rgb.h> //for LED control during splash screen
#include <scheduler.h> //for scheduling animations
#include <config.h> //num LEDs, LED colors, splash screen timing defaults

class Splash_Screen :  public UI_Page {
public:
    //splash screen takes the graphics handle and some LEDs to animate
    //also pass in the page to render after splash screen is complete
    Splash_Screen(UI_Page* _next_page);

    //also provide a default constructor for easier initialization in some circumstances
    Splash_Screen();

    //provide a method of attaching a page if the default constructor was invoked
    void set_next_page(UI_Page* _next_page);

    //change default splash screen timing values
    //normally read in from `App_Constants`
    void configure_timing(uint32_t _led_anim_ms, uint32_t _dwell_im_ms, uint32_t _dwell_dark_ms);

private:
    //override entry function --> schedule splash screen and LED animations
    void impl_on_entry() override;

    //override exit function --> turn off all LEDs
    void impl_on_exit() override;

    //override draw function --> draw the splash screen bitmap
    void draw() override; 

    //private function that runs the LED animation and its forwarding function from a callback
    void animate_LEDs(); 
    static inline void animate_LEDs_cb(void* context) { reinterpret_cast<Splash_Screen*>(context)->animate_LEDs(); }

    //private function that turns off all LEDs
    void clear_LEDs();

    //splash screen image
    static constexpr size_t splash_image_width = 128;
    static constexpr size_t splash_image_height = 64;
    static const std::array<uint8_t, splash_image_width * splash_image_height> U8X8_PROGMEM splash_image;

    //own a blank dwell screen to run for a bit after splash animation
    Blank_Dwell_Screen dwell_screen;

    //maintain a page transition object to go to a blank dwell screen after splash animation completed
    Pg_Transition to_next_page;

    //maintain a scheduler to sequence splash screen animations
    Scheduler splash_sched;

    //maintain timing primitives, initialized to app configuration values
    uint32_t led_anim_ms = App_Constants::SPLASH_SCREEN_LED_DELAY_MS;
    uint32_t dwell_im_ms = App_Constants::SPLASH_SCREEN_DWELL_IMAGE_MS; 
    uint32_t dwell_dark_ms = App_Constants::SPLASH_SCREEN_DWELL_DARK_MS;
};