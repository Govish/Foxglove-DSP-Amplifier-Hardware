#pragma once

/*
 * This class holds a collection of all the effects
 * use this class to create UI menus, create UI menu actions, and load effects themselves
 * Will also hold the collection of active effects in an array of `std::unique_ptr`s
 * 
 * Intention is to use this class statically, i.e. don't instantiate it
 * 
 * By Ishaan Gov
 */

#include <memory> //for `unique_ptr`
#include <array>
#include <Arduino.h>

#include <effect_interface.h> //hold container of effects
#include <config.h> //for constants

//typedef outta convenience
typedef std::array<std::unique_ptr<Effect_Interface>, App_Constants::NUM_EFFECTS> Active_Effects_t;

class Effects_Manager {
public:
    //prevent all flavors of making an instance of one of these
    Effects_Manager() = delete;
    Effects_Manager(const Effects_Manager& other) = delete;
    void operator=(const Effects_Manager& other) = delete;

    //initialize the active effects
    static void init();

    //return the number of available effects
    static inline size_t get_num_effects() { return NUM_AVAIALBLE_EFFECTS; }

    //replace the effect at the specified index with the effect from our list at the speficied index
    static void replace(size_t effect_index, size_t effect_no_in_list);

    //get the names of all the available effects, in the order of their indices
    //would like to return a reference to a std::array but compile-time array size computation is unavailable
    //next best thing is an `App_span` which will hopefully have a similar interface
    static App_Span<std::string> get_available_names();

    //individual and collective getter functions for the active effects
    static inline std::unique_ptr<Effect_Interface>& get_active_effect(size_t i) { return active_effects[i]; }
    static inline Active_Effects_t& get_active_effects() { return active_effects; }

private:
    //have a collection of all possible effects
    //can't directly own instances of a base class since that doesn't allow for polymorphic behavior
    //instead we have to own pointers to effects
    //"prototypes" of these effects will be created on the heap (i.e. a single copy of each effect)
    //  \--> these will exist throughout the lifetime of the program and won't be touched
    //  \--> as such, c-style pointers should be fine for something like this
    //making this a c-style container since it's tricky to automatically deduce the size for a std::array
    static Effect_Interface* const available_effects[];
    static const size_t NUM_AVAIALBLE_EFFECTS;

    //most importantly, hold an array of `std::unique_ptr`s to active effects
    static Active_Effects_t active_effects;
};