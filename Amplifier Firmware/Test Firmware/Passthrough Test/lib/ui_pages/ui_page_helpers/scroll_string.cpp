
#include "scroll_string.h"

//default constructor does nothing
Scroll_String::Scroll_String() {}

//initialize the object with specific render text
Scroll_String::Scroll_String(std::string _text_to_render) 
{
    set_render_text(_text_to_render);
}

//initialize the object with render text AND a position on the screen
Scroll_String::Scroll_String(   std::string _text_to_render, 
                                u8g2_uint_t _top_y, 
                                u8g2_uint_t _bottom_y, 
                                u8g2_uint_t _left_x, 
                                u8g2_uint_t _right_x) 
{
    set_render_text(_text_to_render);
    set_bounding_box(_top_y, bottom_y, _left_x, _right_x);
}

//just save the passed text into the member variable
void Scroll_String::set_render_text(std::string _text_to_render) {
    text_to_render = _text_to_render;
}

//just save the coordinates in to the local variables
void Scroll_String::set_bounding_box(   u8g2_uint_t _top_y, 
                                        u8g2_uint_t _bottom_y, 
                                        u8g2_uint_t _left_x, 
                                        u8g2_uint_t _right_x) 
{
    //sanity check inputs
    if(_bottom_y < _top_y) return; 
    if(_right_x < _left_x) return;

    //save to member variables
    bottom_y = _bottom_y;
    top_y = _top_y;
    left_x = _left_x;
    right_x = _right_x;
}

//reset the rendering state variable and schedule the enable scrolling callback function
void Scroll_String::start(bool _enable_scrolling) {
    //just reset the scrolling state variable
    //save whether we want to enable scrolling
    //reset our text offset
    //and reactivate the scheduler that sets this flag
    do_scroll = false;
    enable_scrolling = _enable_scrolling;
    scroll_sched.schedule_oneshot_ms(   Context_Callback_Function<void>(reinterpret_cast<void*>(this), do_scroll_cb),
                                        App_Constants::SCROLLING_TEXT_DWELL_MS);
}

//deschedule the scrolling-enable callback function
//reset our text offset, and reset our enable flag
void Scroll_String::stop() {
    scroll_sched.deschedule();
    enable_scrolling = false;
    text_x_offset = 0; //reset out text offset 
}

//render the text on the screen
//text will left align horizontally in the bounding box
//and center align vertically in the bounding box
//all text will be cropped to fit in the bounding box accordingly
//NOTE: this function won't clear the display buffer!
//      ADDITIONALLY, set the render font before calling this function!
void Scroll_String::render(U8G2& graphics_handle) {
    
    //################### COMPUTE THE HORIZONTAL LOCATION OF OUR TEXT ###################

    //check whether our text can fit in our bounding box
    u8g2_uint_t text_width = graphics_handle.getStrWidth(text_to_render.c_str());
    u8g2_uint_t text_width_plus_pad = graphics_handle.getStrWidth((text_to_render + "    ").c_str());
    bool text_fits = ( text_width <= (right_x - left_x) );
    bool scrolling = !text_fits && enable_scrolling && do_scroll;
    
    //inspired by U8G2 scrolling text example
    //NOTE: IN THAT EXAMPLE, ALL THE SCROLLING MATH IS USING UNSIGNED VALUES, WHICH STILL SEEMS TO WORK
    //I'm imagining they're explointing rollover arithmetic which yeah a little weird but hey seems to work
    if(scrolling) {
        //we're scrolling; move over by a single pixel
        text_x_offset -= scroll_px_per_update;

        //wrap around once our offset reaches the width of our text + padding
        if(text_x_offset <= -(float)(text_width_plus_pad))
            text_x_offset = 0;
    }

    //################### ACTUALLY DRAW OUR TEXT ###################

    //we'll clip our buffer write area to the bounding box  
    graphics_handle.setClipWindow(left_x, top_y, right_x, bottom_y);

    //compute our pixel coordinates of the text we wanna render
    //text can automatically center align, so just compute the vert center of our box
    u8g2_uint_t vert_center = (bottom_y + top_y) >> 1;
    u8g2_uint_t horiz_left = left_x + ((u8g2_uint_t) - (u8g2_uint_t)(-1 * text_x_offset)); //have to do some messy unsigned math here
    
    //draw two copies of the font at the computed positions
    //draw another copy of the text if we're scrolling --> gives effect of wrapping around
    graphics_handle.setFontPosCenter(); //vertically center our text
    graphics_handle.drawStr(horiz_left, vert_center, text_to_render.c_str());
    if(scrolling)
        graphics_handle.drawStr(horiz_left + text_width_plus_pad, 
                                vert_center, text_to_render.c_str());    

    //bring font and screen clipping back to default
    graphics_handle.setFontPosBaseline(); 
    graphics_handle.setMaxClipWindow();

}