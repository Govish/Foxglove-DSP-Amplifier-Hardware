#pragma once
/* 
 * Simple class that packages and streamlines writing to the RGB LEDs 
 *
 * Ishaan Gov Dec 2023
 */

#include <Arduino.h> //for types, interface

class RGB_LED {
public:
    //============= create a struct that packages `analogWrite()` into something convenient ==============
    struct COLOR {
        uint8_t red_val;
        uint8_t green_val;
        uint8_t blue_val;

        //a quick function that returns a color based off R, G, B values 
        static COLOR mk_color(uint8_t r, uint8_t g, uint8_t b) {
            return {r, g, b};
        }

        //and a quick function to blur between colors that's gamma corrected (gamma = 2)
        // fade = 0 --> fully c1, fade = 1 --> fully c2
        static COLOR blend(COLOR c1, COLOR c2, float fade) {
            //constrain fade
            if(fade < 0) fade = 0;
            if(fade > 1) fade = 1;
            
            //blend the color using gamma correction (sqrt, mix, then square)
            COLOR blend;
            blend.red_val = (uint8_t)pow(sqrt((float)c2.red_val) * fade + sqrt((float)c1.red_val) * (1-fade), 2.0f);
            blend.green_val = (uint8_t)pow(sqrt((float)c2.green_val) * fade + sqrt((float)c1.green_val) * (1-fade), 2.0f);
            blend.blue_val = (uint8_t)pow(sqrt((float)c2.blue_val) * fade + sqrt((float)c1.blue_val) * (1-fade), 2.0f);
            
            //return the result
            return blend;
        }
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