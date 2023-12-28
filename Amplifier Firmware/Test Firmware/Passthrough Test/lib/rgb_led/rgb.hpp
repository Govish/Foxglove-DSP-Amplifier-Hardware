#pragma once
/* 
 * Simple class that packages and streamlines writing to the RGB LEDs 
 *
 * Ishaan Gov Dec 2023
 */

#include "Arduino.h"

#include "config.hpp" 

class RGB_LED {
public:
    //============= create a struct that packages `analogWrite()` into something convenient ==============
    struct COLOR {
        uint8_t red_val;
        uint8_t green_val;
        uint8_t blue_val;
    };
    
    //need `inline` for the following defs since C++ version doesn't automatically do this
    //and errors out without the inline (but compiler throws warnings with `inline` for whatever reason)
    static inline constexpr COLOR RED = {255, 0, 0};
    static inline constexpr COLOR GREEN = {0, 255, 0};
    static inline constexpr COLOR BLUE = {0, 0, 255};
    static inline constexpr COLOR YELLOW = {255, 255, 0};
    static inline constexpr COLOR CYAN = {0, 255, 255};
    static inline constexpr COLOR MAGENTA = {255, 0, 255};
    static inline constexpr COLOR WHITE = {255, 255, 255};
    static inline constexpr COLOR OFF = {0, 0, 0};
    
    //some more useful colors that aren't the principle R, G, B ones
    static inline constexpr COLOR PURPLE = {64, 0, 255};
    static inline constexpr COLOR ORANGE = {255, 64, 0};
    //================================ END COLORs ================================

    //delete copy constructor and assignment operator to avoid any weird hardware conflicts
    RGB_LED(const RGB_LED& other) = delete;
    void operator=(const RGB_LED& other) = delete;

    //initialize an RGB LED on the following PWM pins
    //also include polarity, default active LOW polarity
    RGB_LED(const uint8_t red_pin, const uint8_t green_pin, const uint8_t blue_pin, const bool _active_high = false);

    //initialize the necessary PWM channels
    void init();

    //set the color of the RGB LED
    //provide a couple overloads for convenience
    //also set the brightness accordingly
    void set_color(COLOR c, float _brightness = 1.0f);
    void set_color(uint8_t pwm_r, uint8_t pwm_g, uint8_t pwm_b, float _brightness = 1.0f); //0 is off, 255 is max brightness

    //set the brighness of the particular color--mostly a utility function
    void set_brightness(float b);

private:
    //save the pins upon initialization
    const uint8_t r, g, b;
    const bool active_high; //also save the polarity
    
    //set this flag in the constructor if pins are PWM capable
    const bool pwm_capable;

    //save the color being set
    COLOR current_color = OFF; 

    //and also save the brighness level
    float brightness = 1.0f;
};