#pragma once

/* 
 * Simple UI page that clears the screen and waits a little bit
 * Lets you add a little "breath" before jumping to the next menu
 * 
 * By Ishaan Gov Dec 2023
 */

#include <Arduino.h> //types, interface

#include <ui_page.h> //for parent class
#include <scheduler.h> //to run transition after dwell

class Blank_Dwell_Screen : public UI_Page {
public:
    Blank_Dwell_Screen(uint32_t _dwell_time, UI_Page* _next_page);

    //allow changing of the dwell time and the next page
    void set_dwell_time(uint32_t _dwell_time);
    void set_next_page(UI_Page* _next_page);

private:
    //override draw; use this to clear the screen
    void draw() override;

    //override `on_entry()` --> call `draw()` and schedule a transition
    void impl_on_entry() override;

    //maintain a page transition object to go to a blank dwell screen after splash animation completed
    Pg_Transition to_next_page;

    //maintain a scheduler to fire off a transition to the next page
    Scheduler next_page_sched;

    //maintain the dwell time --> how long we'd like to stay at this page before moving on 
    uint32_t dwell_time;
};