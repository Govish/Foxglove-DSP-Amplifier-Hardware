#include "rgb.hpp"

//a chill little constructor that initializes some constants
RGB_LED::RGB_LED(const uint8_t red_pin, const uint8_t green_pin, const uint8_t blue_pin, const bool _active_high):
    r(red_pin), g(green_pin), b(blue_pin), active_high(_active_high),
    pwm_capable(    digitalPinHasPWM(r) && //call the LED PWM capable if all pins are on PWM pins
                    digitalPinHasPWM(g) &&
                    digitalPinHasPWM(b))
{}

//just set pinmodes and resolution in the INIT
void RGB_LED::init() {
    //set all PWM pins to outputs
    pinMode(r, OUTPUT);
    pinMode(g, OUTPUT);
    pinMode(b, OUTPUT);

    //set the PWM resolution to 8 bits; don't need anything more than this
    analogWriteResolution(8);

    //set the PWM frequency to be equal to the audio sample rate
    //PWM introduces noise very audible noise into the chip likely due to high edge rates on the IO pins
    //setting PWM frequency to sample rate causes any of these harmonics to alias down to DC (or low frequency if it's off by a bit)
    //the low frequencies will be more or less blocked by our filters and output circuitry
    analogWriteFrequency(r, App_Constants::AUDIO_SAMPLE_RATE_HZ);
    analogWriteFrequency(g, App_Constants::AUDIO_SAMPLE_RATE_HZ);
    analogWriteFrequency(b, App_Constants::AUDIO_SAMPLE_RATE_HZ);   

    //start the LED with everything off
    set_color(OFF);
}

//set the color to a preset color --> just redirect to the color setting function
void RGB_LED::set_color(COLOR c, float _brightness) {
    set_color(c.red_val, c.green_val, c.blue_val, _brightness);
}

//adjust the brightness of the current color
//set color function does the constraining from 0 --> 1
void RGB_LED::set_brightness(float b) {
    set_color(current_color.red_val, current_color.green_val, current_color.blue_val, b);
}

//set the output PWM values (or DC values if not PWM capable)
void RGB_LED::set_color(uint8_t pwm_r, uint8_t pwm_g, uint8_t pwm_b, float _brightness) {
    //save the PWM and brightness values before we modify them
    current_color = {pwm_r, pwm_g, pwm_b};
    brightness = constrain(_brightness, 0.0f, 1.0f);
    
    //calculate the actual PWM values based off their brightness
    //convert to uint16_t so we can handle full off or full on (I think analogWrite handles this appropriately)
    uint16_t bright_r, bright_g, bright_b;
    bright_r = (uint16_t)((float)pwm_r * brightness);
    bright_g = (uint16_t)((float)pwm_g * brightness);
    bright_b = (uint16_t)((float)pwm_b * brightness);

    //invert the PWM values if we're active low
    if(!active_high) {
        bright_r = 256 - bright_r;
        bright_g = 256 - bright_g;
        bright_b = 256 - bright_b;
    }

    //write the PWM values to the pins if they're PWM capable
    if(pwm_capable) {
        analogWrite(r, bright_r);
        analogWrite(g, bright_g);
        analogWrite(b, bright_b);
    }
    else { //otherwise just toggle their HIGH/LOW values if they're regular GPIO
        digitalWriteFast(r, bright_r > 127);
        digitalWriteFast(g, bright_g > 127);
        digitalWriteFast(b, bright_b > 127);
    }
}