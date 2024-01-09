#pragma once

/*
 * Using this file to basically create, initialize and run the UI system
 * This keeps the main loop from getting toooooo cluttered
 * 
 * Intention is to use this class statically -- It shouldn't be possible to create any instances of said class
 *  - call the `make_ui` function to create and initialize some pages
 *  - call the `start` function to enter the UI system 
 * Running the UI system basically requires regular calls to the encoder and scheduler update functions, but that's about it
 * UI system will manage everything else by itself
 * 
 * By Ishaan Gov Jan 2024
 */

#include <Arduino.h>
#include <U8g2lib.h>

//==== UI PAGE INCLUDES ====
#include <ui_page.h>

class UI_System {
public:
    //don't allow any flavor of instantiation
    UI_System() = delete;
    UI_System(const UI_System& other) = delete;
    void operator=(const UI_System& other) = delete;

    //have a function that sets up all the UI pages
    static void make_ui();

    //have a function that starts the UI system
    //enter the UI system through the point configured in make_ui()
    static inline void start() { if(entry != nullptr) entry->on_entry(); }

private:
    //select effect forwarding function
    //use this function to select a new effect for the particular effect index
    //expects `context` to point to a std::pair<size_t, size_t>
    //  \--> first element corresponds to index
    //  \--> second element corresponds to new effect number
    static void replace_effect_cb(void* context);

    //this is our entry point into the UI system
    //set this variable only after make_ui() has been called
    static UI_Page* entry;

    //a little messy, but have a pointer to the main screen
    //use this to transition back to the main screen after choosing an effect
    static UI_Page* app_main_screen;
};

