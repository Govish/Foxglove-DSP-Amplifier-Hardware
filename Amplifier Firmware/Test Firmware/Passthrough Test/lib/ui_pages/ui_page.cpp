
#include <ui_page.h>

//========================= STATIC VARIABLE DEFINITION ========================

//run the default constructor for the draw scheduler
//shared by all UI pages, but only the active page uses it at any given time
Scheduler UI_Page::draw_sched;

//hold a pointer to the active UI_Page
//will be initialized on entry into the UI system, don't need to worry too much about nullptr
UI_Page* UI_Page::active_page = nullptr;

//========================= PUBLIC FUNCTIONS ========================

//function that gets called when this page is loaded
//anticipate calling this when going from a different page to this one (will be called automatically by `Pg_Transition` class)
void UI_Page::on_entry() {
    //we're the active page now
    active_page = this;

    //call the draw function at the rate specified by the app configuration
    draw_sched.schedule_interval_ms(    Context_Callback_Function<void>(reinterpret_cast<void*>(this), draw_cb),
                                        App_Constants::SCREEN_REDRAW_MS);

    //run the child's implementation of `on_entry()`
    this->impl_on_entry();
}

//function that gets called when we're leaving this page
//anticipate calling this when going from this page to a different one (will be called automatically by `Pg_Transition` class)
void UI_Page::on_exit() {
    //stop drawing the screen 
    draw_sched.deschedule();

    //run the child's implementation of `on_exit()`
    this->impl_on_exit();        
}