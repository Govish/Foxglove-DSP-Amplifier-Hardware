
#include "menu_full_screen.h"

Menu_Full_Screen::Menu_Full_Screen():
    UI_Menu() //call the default constructor of our base class
{}

Menu_Full_Screen::Menu_Full_Screen(size_t knob_led_index, UI_Page* _prev_page, UI_Menu_Item& _back_item):
    UI_Menu(knob_led_index, _prev_page, _back_item) //just initialize the base class
{}

//setter method for the header text when we render our screen
void Menu_Full_Screen::set_header_text(std::string _header_text) {
    header_text = _header_text;
}

//actual menu rendering function
//TODO: FIX --> still some bugs with frame drawing, active area
void Menu_Full_Screen::menu_draw() {
    //start by clearing the display buffer
    graphics_handle.clearBuffer();

    //############## HEADER AND UNDERBAR ##############

    //draw the header text at the top of the page; compute some constants for doing so
    u8g2_uint_t text_height = graphics_handle.getAscent() - graphics_handle.getDescent();    
    static const u8g2_uint_t HEADER_TEXT_X = 0;
    static const u8g2_uint_t HEADER_TEXT_Y = 0;

    graphics_handle.setFontPosTop(); //use the centerline of the text to place it
    graphics_handle.drawStr(HEADER_TEXT_X, HEADER_TEXT_Y, header_text.c_str());
    graphics_handle.setFontPosBaseline(); //restore to default

    //draw a horizontal bar underneath the header
    u8g2_uint_t UNDERBAR_Y = text_height + 1;
    graphics_handle.drawHLine(0, UNDERBAR_Y, graphics_handle.getDisplayWidth());

    //############## MENU ITEMS ##############
    //establish the amount of padding around each menu item
    //each of these values will be increased by 1 to accommodate the frame
    static const u8g2_uint_t MENU_ITEM_PADDING_X = 1;
    static const u8g2_uint_t MENU_ITEM_PADDING_Y = 2;
    static const u8g2_uint_t ACTIVE_REGION_START_X = MENU_ITEM_PADDING_X + 1;
    static const u8g2_uint_t ACTIVE_REGION_END_X = graphics_handle.getDisplayWidth() - (MENU_ITEM_PADDING_X + 1);
    u8g2_uint_t ACTIVE_REGION_START_Y = UNDERBAR_Y;
    static const u8g2_uint_t ACTIVE_REGION_END_Y = graphics_handle.getDisplayHeight();
    
    //narrow the drawing window to an area below the header with vertical padding for columns
    graphics_handle.setClipWindow(  ACTIVE_REGION_START_X, ACTIVE_REGION_START_Y, 
                                    ACTIVE_REGION_END_X, ACTIVE_REGION_END_Y);

    //get the height of the selected item; and draw it at the center of the active region
    uint32_t selected_height = selected->get_height(graphics_handle);
    u8g2_uint_t selected_start = ((ACTIVE_REGION_END_Y - ACTIVE_REGION_START_Y - selected_height) >> 1) + ACTIVE_REGION_START_Y;
    selected->draw(graphics_handle, ACTIVE_REGION_START_X, selected_start,
                                    ACTIVE_REGION_END_X, selected_start + selected_height);

    //draw items below until (we leave the screen) or (we don't have next)
    //start by grabbing the item below us and computing its starting y coordinate
    UI_Menu_Item* below = selected->get_next();
    u8g2_uint_t below_start = selected_start + selected_height + (2*MENU_ITEM_PADDING_Y + 1);
    while(below != nullptr && below_start <= ACTIVE_REGION_END_Y) {
        //get the render height of the particular item
        u8g2_uint_t below_height = below->get_height(graphics_handle);
        
        //actually draw the item below
        below->draw(graphics_handle,    ACTIVE_REGION_START_X, below_start,
                                        ACTIVE_REGION_END_X, below_start + below_height);
        
        //then grab the next item and compute its starting y coordinate
        below = below->get_next();
        below_start += below_height + (2*MENU_ITEM_PADDING_Y + 1);
    }
        
    //draw items above until (we leave the screen) or (we dont' have next)
    //start by grabbing the item above us and computing its ending y coordinate
    //going up is a little dicey since values can be negative
    //as such our `end coordiate` is gonna be stored as an int32 to handle rollover
    UI_Menu_Item* above = selected->get_prev();
    int32_t above_end = selected_start - (2*MENU_ITEM_PADDING_Y + 1);
    while(above != nullptr && above_end >= (int32_t)ACTIVE_REGION_START_Y) {
        //get the render height of the particular item
        u8g2_uint_t above_height = above->get_height(graphics_handle);
        
        //actually draw the item below
        above->draw(graphics_handle,    ACTIVE_REGION_START_X, (u8g2_uint_t)(above_end - above_height),
                                        ACTIVE_REGION_END_X, above_end);
        
        //then grab the next item and compute its starting y coordinate
        above = above->get_next();
        above_end -= above_height + (2*MENU_ITEM_PADDING_Y + 1);
    }

    //############## SELECTED ITEM ##############
    //draw a frame around our selected item, computing some constants first
    static const u8g2_uint_t frame_x_start = 0;
    static const u8g2_uint_t frame_width = graphics_handle.getDisplayWidth();
    u8g2_uint_t frame_y_start = selected_start - MENU_ITEM_PADDING_Y - 1;
    u8g2_uint_t frame_height = selected_height + 2*(MENU_ITEM_PADDING_X + 1);

    graphics_handle.drawRFrame(frame_x_start, frame_y_start, frame_width, frame_height, 1);

    //############## RESTORE SETTINGS AND SEND BUFFER ##############
    graphics_handle.setMaxClipWindow();
    graphics_handle.sendBuffer();
}