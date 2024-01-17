
#include <all_effects.h>

#include <audio_out_mqs.h> //need this to pause the audio system update

//######## EFFECTS INCLUDES #########
#include <effect_test_passthrough.h>
#include <effect_test_params.h>
#include <effect_iir_lp.h>

//======================== STATIC VARIABLE DEFINITION =====================
//################### USE THIS SPACE TO INSTANTIATE "MASTERs" OF ALL EFFECTS #################

Effect_Interface* const Effects_Manager::available_effects[] = {
        //initialize some passthrough tests
        new Effect_Test_Passthrough(RGB_LED::WHITE, "Default Passthrough"),

        //Initialize passthrough test with param
        new Effect_Test_Param(RGB_LED::RED, "Passthrough Param"),

        //Initialize a prototype lowpass FIR and IIR filter
        /* TODO FIR filter */
        new Effect_IIR_LP(),
    };

//################### end EFFECT MASTER DEFINITION #####################

//have a convenience variable that knows the number of available effects
const size_t Effects_Manager::NUM_AVAIALBLE_EFFECTS = sizeof(available_effects) / sizeof(available_effects[0]); 

//declare the effects manager array whatever default values; properly initialized in `init()` below
Active_Effects_t Effects_Manager::active_effects = {};

//================================= PUBLIC MEMBER FUNCTIONS =============================

//initialize the active effects array
//just make copies of the first audio effect in our list as a default
void Effects_Manager::init() {
    for(auto& effect : active_effects) 
        effect = available_effects[0]->clone();
}

//replace the effect at `effect_index` with a clone of the effect at `effect_no_in_list`
//call `connect()` and `disconnect()` as necessary
void Effects_Manager::replace(size_t effect_index, size_t effect_no_in_list) {
    //sanity check the inputs, return if they're outta range
    if(effect_index >= active_effects.size()) return;
    if(effect_no_in_list >= NUM_AVAIALBLE_EFFECTS) return;
    
    //pause the audio system update --> ensures no funky race conditions
    //TODO: manage detaching and reattaching effects in a way that avoids clicks and pops
    Audio_Out_MQS::pause_interrupt();

    //disconnect the "outgoing" effect
    active_effects[effect_index]->disconnect();

    //make a copy of our master effect in our list
    active_effects[effect_index] = available_effects[effect_no_in_list]->clone();

    //and connect the effect to the system
    active_effects[effect_index]->connect();

    //ensure all memory addresses of the effects are synchronized
    //ensures no invalid memory accesses once audio update interrupt is resumed
    //might not be necessary --> put this here during some bug-chasing
    // asm volatile("dsb");

    //resume the audio interrupt
    Audio_Out_MQS::resume_interrupt();
}

//get the names of the available effects
App_Span<std::string> Effects_Manager::get_available_names() {
    //maintain a statically allocated array of effect names
    //return a pointer to this array and its size 
    static std::array<std::string, NUM_AVAIALBLE_EFFECTS> effect_names;
    static bool initialized = false; //flag to only do this work once 
    
    //if we haven't initialized the array yet
    //grab the names of each element and store them in the corresponding index
    //set the initialized flag too
    if(!initialized) {
        for(size_t i = 0; i < NUM_AVAIALBLE_EFFECTS; i++)
            effect_names[i] = available_effects[i]->get_name();
        initialized = true;
    }

    //return a "DIY std::span" that refers to this array we've created
    return App_Span<std::string>(effect_names);
}