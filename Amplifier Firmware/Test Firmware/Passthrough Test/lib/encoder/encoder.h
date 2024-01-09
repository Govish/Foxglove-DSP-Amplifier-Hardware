#pragma once
/*
 * Quick class to handle reading rotary encoders
 * Designed for use in our particular application, so has some more specialized functionality
 * Also sacrifices some code portability for the sake of cleanliness (specific timer that the encoder is sampled on)
 * 
 * NOTE: This function takes over the Periodic Interrupt Timer (PIT) and PIT Channel 1
 *      --> MODIFY ISR IF THERE'S OTHER FUNCTIONS THAT MAY NEED ACCESS TO THIS TIMER 
 * 
 * By Ishaan Gov 2023
 */

#include <array> //to hold the count mode LUTs
#include <Arduino.h> //for types, pin utilities

#include <config.h> //for num encoders
#include <utils.h> //for callback functions

class Rotary_Encoder {
public:
    //======================== ENCODER COUNTING LOOKUP TABLES ===========================
    /*
     * Depending on the counting strategy and direction of the encoder, have different lookup tables available
     * This is basically the core functionality of this library, so I'll try to explain a little bit:
     * 
     * All the encoder inputs (A, B, SW) are sampled at a specific interval
     * We'll handle the switch input separately from the encoder, since they're basically independent. 
     * Everything below pertains to just the encoder inputs (A, B):
     *      We'll store the present and previous state in a memory efficient way by throwing it all into an 8-bit integer
     *      It'll have the following format: (x = don't care)
     *                                  x   x   x   x   [PREV_A]    [PREV_B]    [A]     [B]
     *      There are are 16 possible values of this uint8_t in the part that we care about
     *      And in certain combinations of each of PREV_A, PREV_B, A, B, we'll increment, decrement, or do nothing to our encoder count
     *      That information (whether we inc/dec/do nothing to the count) can be stored into a 16-element lookup table
     *      That lookup table can be different depending on our counting strategy and whether we're inverting directions
     * 
     * Regarding the switch input, we basically just look at the previous and present value, and do something if they're different
     * 
     * If we sample the encoder/switches at invervals longer than their bounce time, we shouldn't have to worry about debouncing!
     */

    //declaring a Lookup table type for the encoders for convenience
    typedef std::array<int8_t, 16> ENC_LUT_t;

    static inline constexpr ENC_LUT_t ENC_LUT_1X_FORWARD = {   
        0, 0, -1, 0,
        0, 0, 0, 1,
        0, 0, 0, 0,
        0, 0, 0, 0 };
    
    static inline constexpr ENC_LUT_t ENC_LUT_2X_FORWARD = {
        0, 0, -1, 0,
        0, 0, 0, 1,
        1, 0, 0, 0,
        0, -1, 0, 0 };

    static inline constexpr ENC_LUT_t ENC_LUT_4X_FORWARD = {  
        0, 1, -1, 0,
        -1, 0, 0, 1,
        1, 0, 0, -1,
        0, -1, 1, 0 };

    static inline constexpr ENC_LUT_t ENC_LUT_1X_REVERSE= {   
        0, 0, 1, 0,
        0, 0, 0, -1,
        0, 0, 0, 0,
        0, 0, 0, 0 };
    
    static inline constexpr ENC_LUT_t ENC_LUT_2X_REVERSE = {
        0, 0, 1, 0,
        0, 0, 0, -1,
        -1, 0, 0, 0,
        0, 1, 0, 0 };

    static inline constexpr ENC_LUT_t ENC_LUT_4X_REVERSE = {  
        0, -1, 1, 0,
        1, 0, 0, -1,
        -1, 0, 0, 1,
        0, 1, -1, 0 };

    
    //creating an array of these LUTs so we can index into them (makes the constructor more readable)
    static inline constexpr std::array<ENC_LUT_t, 6> ENC_LUTs = {
        ENC_LUT_1X_FORWARD, ENC_LUT_2X_FORWARD, ENC_LUT_4X_FORWARD, 
        ENC_LUT_1X_REVERSE, ENC_LUT_2X_REVERSE, ENC_LUT_4X_REVERSE,
    };

    //and creating an enum that allows us to only select one of these LUTs --> ensure user can't pass in a weird LUT
    enum cnt_dir_t {
        X1_FWD = 0, 
        X2_FWD = 1,
        X4_FWD = 2,
        X1_REV = 3,
        X2_REV = 4,
        X4_REV = 5
    } ENC_COUNT_AND_DIR;

    //============================ PUBLIC FUNCTIONS ============================

    //instantiate an encoder that reads the following pins, and counts with the specified properties
    Rotary_Encoder(const uint8_t pin_a, const uint8_t pin_b, const uint8_t pin_sw, const cnt_dir_t cnt_dir);

    //delete copy constructor and assignment operator to avoid any unintended weird behavior
    Rotary_Encoder(const Rotary_Encoder& other) = delete;
    void operator=(const Rotary_Encoder& other) = delete;

    //initialize the hardware, and sets up the GPIO sampling as necessary
    void init();

    /*
     * Setter and getter methods for counts and switch states
     * Re: counts, allow for adjustable MAX value
     *      i.e. encoder will count from 0 to MAX
     *          \--> doing so simplifies implementation of encoder
     *          \--> user function can rescale this value to the approprate range as necessary
     *      Allow setting of this max value and setting of encoder count
     * Re: switch, just provide the ability to read the current switch state
     */
    bool get_switch(); //TRUE --> pressed
    int32_t get_counts(); //return encoder counts (int32_t to avoid casting)
    void set_max_counts(int32_t _max_counts, int32_t reset_val = 0); //reset the max allowable counts to a new value
    void set_counts(int32_t counts); //force the encoder counts to be a particular value; DON'T fire `on_change` interrupt


    /*
     * Attach callback functions --> these function will execute when `update()` is called if...
     *   `attach_on_change()` --> the encoder count has changed since last call to update
     *   `attach_on_press()` --> the pushbutton has been pressed since last call to update
     *   `attach_on_release()` --> the pushbutton has been released since last call to update
     */
    void attach_on_change(Context_Callback_Function<void> _on_change);
    void attach_on_press(Context_Callback_Function<void> _on_press);
    void attach_on_release(Context_Callback_Function<void> _on_release);


    /*
     * Update function --> executes callback functions as necessary when this function is called 
     * `update()` just calls the update on a single encoder instance
     * `update_all()` calls updates on all encoder instances (and is a static method)
     */
    void update();
    static void update_all();

private:
    //save the pins of the encoder
    const uint8_t a, b, sw;
    
    //which counting lookup table we're gonna use, based on count mode and direction
    const ENC_LUT_t& COUNT_LUT; 
    static const uint8_t ENC_HISTORY_LUT_MASK = 0x0F; //only care about the last 4 bits of encoder history

    //only sample the switch every couple of encoder samples
    //allows the switch to bounce longer than the encoder
    static const size_t switch_sample_interval = (size_t)(App_Constants::ENC_SW_BOUNCE_TIME / App_Constants::ENC_BOUNCE_TIME + 0.5);
    size_t switch_sample_counter = switch_sample_interval - 1;

    //memory variables for the switch states and encoder counts
    bool last_switch, current_switch;
    uint8_t encoder_history;
    int32_t encoder_count = 0;
    int32_t last_encoder_count = 0;
    int32_t encoder_max_count = 0;
    
    //flag variables about whether certain events have taken place
    bool flag_change = false;
    bool flag_press = false;
    bool flag_release = false;  

    //callback functions
    Context_Callback_Function<void> on_change;
    Context_Callback_Function<void> on_press;
    Context_Callback_Function<void> on_release;

    //functions to sample the encoder inputs
    static bool timer_initialized; //flag to way whether the one-time things have been initialized
    static void SAMPLE_ISR(); //called by a timer interrupt (or at a fixed interval) --> calls `sample()` on all instances
    void sample(); //sample update for a single instance

    //container to hold pointers to all created encoder instances
    //avoiding using `vector` for heap allocation reasons
    //use a fixed-size container whose size is set by config
    static std::array<Rotary_Encoder*, App_Constants::NUM_ENCODERS> ALL_ENCODERS;
    static size_t num_created_instances;
};