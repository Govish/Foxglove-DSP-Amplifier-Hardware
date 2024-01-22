#include <idle_screen.h>

//constructor to basically set up some member variables
Idle_Screen::Idle_Screen(UI_Page* _next_page):
    to_next_page(_next_page) //set up the page transition
{}


void Idle_Screen::set_next_page(UI_Page* _next_page) {
    to_next_page.set_to(_next_page);
}

//on page entry, just draw the page (i.e. clear the screen) and schedule the page transition
void Idle_Screen::impl_on_entry() {
    //set up all encoders to return to the previous page on any kinda event
    for(auto& enc : encs) {
        enc->set_max_counts(2, 1); //allow for a turn in either direction to bring back to the previous page
        enc->attach_on_change(to_next_page);
        enc->attach_on_press(to_next_page);
    }

    //turn off all LEDs
    for(auto& led : leds) 
        led->set_color(RGB_LED::OFF);

    //draw the message that we're on the idle screen, and to turn or click the encoders to return
    //start by clearing the display buffer
    graphics_handle.clearBuffer();

    //############## HEADER AND UNDERBAR ##############
    static const std::string IDLE_HEADER = "Idle Screen";
    
    //draw the header text at the top of the page; compute some constants for doing so
    u8g2_uint_t text_height = graphics_handle.getAscent() - graphics_handle.getDescent();    
    static const u8g2_uint_t HEADER_TEXT_X = 0;
    static const u8g2_uint_t HEADER_TEXT_Y = 0;

    graphics_handle.setFontPosTop(); //use the centerline of the text to place it
    graphics_handle.drawStr(HEADER_TEXT_X, HEADER_TEXT_Y, IDLE_HEADER.c_str());
    graphics_handle.setFontPosBaseline(); //restore to default

    //draw a horizontal bar underneath the header
    u8g2_uint_t UNDERBAR_Y = text_height + 1;
    graphics_handle.drawHLine(0, UNDERBAR_Y, graphics_handle.getDisplayWidth());

    //############## IDLE MESSAGE ##############
    static const std::array<std::string, 6> IDLE_MSG = {
            "This page reduces noise",
            "by turning off LEDs",
            "and halting the screen",
            "",
            "Turn/click effect knobs",
            "to return to home page"
        };
    
    static const u8g2_uint_t MSG_TEXT_X = 5;
    u8g2_uint_t MSG_TEXT_Y = UNDERBAR_Y + 2;

    UI_Page::apply_font_small_params();
    text_height = graphics_handle.getAscent() - graphics_handle.getDescent(); 
    graphics_handle.setFontPosTop(); //use the centerline of the text to place it
    for(auto& text : IDLE_MSG) {
        graphics_handle.drawStr(MSG_TEXT_X, MSG_TEXT_Y, text.c_str());
        MSG_TEXT_Y += text_height + 1;
    }
    graphics_handle.setFontPosBaseline(); //restore to default

    //########### update the screen ############
    graphics_handle.sendBuffer();
    UI_Page::restore_font_default();

}

void Idle_Screen::impl_on_exit() {
    //detach all encoder interrupts -- all we need to do
    for(auto& enc : encs) {
        enc->attach_on_press({});
        enc->attach_on_change({});
    }

    //clear the screen
    graphics_handle.clearBuffer();
    graphics_handle.sendBuffer();
}
