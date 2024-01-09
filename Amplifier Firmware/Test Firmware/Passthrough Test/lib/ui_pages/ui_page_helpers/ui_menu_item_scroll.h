#pragma once

/**
 * UI Menu item that displays scrolling text
 * And runs a particular callback function when selected
 * Inherits from `scroll_string` and `ui_menu_item` 
 * Idea is to create these 
 * 
 * By Ishaan Govindarajan
*/
#include <string>
#include <Arduino.h>
#include <U8g2lib.h>

#include <ui_menu_page.h>
#include <ui_page_helpers/scroll_string.h>

//scrolling menu item inherits from a normal menu item
//basically just need to implement constructor, `draw()` and `get_height()`
class Menu_Item_Scroll : public UI_Menu_Item {
public:
    //provide a default constructor and a constructor that initializes the scrolling text
    Menu_Item_Scroll();
    Menu_Item_Scroll(std::string _render_text);

    //provide a way to set the text if default constructor is used
    //just forward it to the scroll string class
    void set_render_text(std::string _text_to_render);

    //override these functions from the menu item class
    //NOTE: render font MUST be set before calling draw!
    void draw(U8G2& graphics_handle, u8g2_uint_t x_left, u8g2_uint_t y_top, u8g2_uint_t x_right, u8g2_uint_t y_bot) override;
    u8g2_uint_t get_height(U8G2& graphics_handle) override;

    //NOTE: additionally have `attach_on_select()` from base class

private:
    //provide implementations for `focus()` and `defocus()`
    //these will just start and stop the scrolling string
    void impl_focus() override;
    void impl_defocus() override;

    //own a scrolling string for rendering
    Scroll_String scroller;

    //save what font we'd like to render the text with
    //and the height of the font for our `get_height()` function
    const uint8_t* font;
    uint32_t item_height = 0;
};





