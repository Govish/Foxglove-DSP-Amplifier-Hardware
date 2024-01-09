#include <blank_dwell_screen.h>

//constructor to basically set up some member variables
Blank_Dwell_Screen::Blank_Dwell_Screen(uint32_t _dwell_time, UI_Page* _next_page):
    to_next_page(_next_page), //set up the page transition
    next_page_sched(), //initilialize the scheduler
    dwell_time(_dwell_time) //save the dwell time
{}

//setter/updater methods
void Blank_Dwell_Screen::set_dwell_time(uint32_t _dwell_time) {
    dwell_time = _dwell_time;
}

void Blank_Dwell_Screen::set_next_page(UI_Page* _next_page) {
    to_next_page.set_to(_next_page);
}

//on page entry, just draw the page (i.e. clear the screen) and schedule the page transition
void Blank_Dwell_Screen::impl_on_entry() {
    //schedule page transition after dwell time
    next_page_sched.schedule_oneshot_ms(to_next_page, dwell_time); 
}

void Blank_Dwell_Screen::draw() {
    //just clear the screen when we draw
    graphics_handle.clearBuffer();
    graphics_handle.sendBuffer();
}