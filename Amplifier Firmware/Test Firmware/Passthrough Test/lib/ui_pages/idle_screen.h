#pragma once

/* 
 * Simple UI page that deactivates all LEDs and doesn't render the screen
 * Experimenting with this to reduce noise of the system
 * 
 * By Ishaan Gov Jan 2024
 */

#include <string>
#include <Arduino.h> //types, interface

#include <ui_page.h> //for parent class
#include <scheduler.h> //to run transition after dwell

class Idle_Screen : public UI_Page {
public:
    Idle_Screen(UI_Page* _next_page);

    //allow changing of the dwell time and the next page
    void set_next_page(UI_Page* _next_page);

private:
    //NOTE: not overriding draw function
    //will do nothing instead of drawing on the screen

    //override `on_entry()` and `on_exit()` --> attach and detach encoder interrupts
    void impl_on_entry() override;
    void impl_on_exit() override;

    //maintain a page transition object to go to a blank dwell screen after splash animation completed
    Pg_Transition to_next_page;
};