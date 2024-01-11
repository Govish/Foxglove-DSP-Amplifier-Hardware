#pragma once

/*
 * Helper class to render scrolling text on the display
 * Provide functions to configure the text and coordinates/bounding box in which to display
 * Then call `start()`, `stop()` and `render()` to run the scrolling
 *      \--> `start()` resets a scheduler that determines when to actually start scrolling the text
 *      \--> `stop()` ensures the scheduler gets descheduled when we no longer care about the item that's scrolling
 *      \--> `render()` actually draws the text; will scroll by the specified increment amount every successive call to this function
 * Use this function wherever scrolling text is needed
 *      \--> main page
 *      \--> UI menus
 *      \--> parameters
 * 
 * By Ishaan Gov Jan 2024 
 */

#include <string>

#include <Arduino.h>
#include <U8g2lib.h>

#include <scheduler.h> //for when to start scrolling text 

class Scroll_String {
public:
    //default constructor to easily initialize an array of these
    //non-default constructor to specify text and/or location on startup
    Scroll_String();
    Scroll_String(std::string _text_to_render);
    Scroll_String(  std::string _text_to_render, 
                    u8g2_uint_t _top_y, 
                    u8g2_uint_t _bottom_y, 
                    u8g2_uint_t _left_x, 
                    u8g2_uint_t _right_x);

    //set the string to render
    void set_render_text(std::string _text_to_render);

    //set how many pixels to scroll per screen update
    inline void set_scroll_px_per_update(float px) { if(px > 0) scroll_px_per_update = px; }

    //set the bounding box in which to render the text
    //text will be horizontally aligned `left` in the bounding box
    //      and vertically aligned `center`
    //NOTE: GIVEN HOW SCREEN COORDINATES WORK, BOTTOM > TOP, RIGHT > LEFT
    //  \--> function will ignore the input if these conditions aren't met
    void set_bounding_box(u8g2_uint_t _top_y, u8g2_uint_t _bottom_y, u8g2_uint_t _left_x, u8g2_uint_t _right_x);

    //reset the rendering state variables and schedule the callback that enables scrolling after dwell
    //intent is to call this function from `on_focus()` when using scrolling text for menus
    void start(bool _enable_scrolling = true);

    //this function is basically to deschedule the scrolling callback function
    //and force disables scrolling of the text
    //ensures that if we're going through menus quickly, we don't get a bunch of callback functions queued up
    //intent is to call this function from `on_defocus()` when using scrolling text for menus
    void stop();

    //actually draw the text on the screen
    //flag to set whether we're actually gonna scroll the item on the screen or not
    //  \--> useful when rendering "defocused" items in menus
    //NOTE: set the font to render before calling this particular function
    void render(U8G2& graphics_handle);

private:
    //scheduler callback function that sets the `do_scroll` flag
    static inline void do_scroll_cb(void* context) {
        reinterpret_cast<Scroll_String*>(context)->do_scroll = true;
    }

    //this is the actual string that we're going to render
    std::string text_to_render = "";

    //delay scrolling of text by a little bit after `reset()` is called
    Scheduler scroll_sched;

    //flag that determines whether we scroll text or not
    //along with a horizontal offset amount useful to determine the pixel coordinate of our scrolled text
    //reset by `reset()`, set by scheduler callback
    float text_x_offset = 0;
    float scroll_px_per_update = 1;
    bool do_scroll = false;
    bool enable_scrolling = true;

    //bounding box for the text rendering
    u8g2_uint_t top_y = 0;
    u8g2_uint_t bottom_y = 0;
    u8g2_uint_t left_x = 0;
    u8g2_uint_t right_x = 0;
};