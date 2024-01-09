#pragma once

#include <string>
#include <Arduino.h>
#include <U8g2lib.h>

#include <ui_menu_page.h> //inherit from here

class Menu_Full_Screen : public UI_Menu {
public:
    //default constructor --> just call the default constructor of the parent
    Menu_Full_Screen();
    
    //non-default constructor that just forwards to `UI_Menu`
    //just forward to the constructor of the `UI_Menu`
    Menu_Full_Screen(size_t knob_led_index, UI_Page* _prev_page, UI_Menu_Item& _back_item);

    //set the header text at the top left of the menu
    void set_header_text(std::string _header_text);

private:
    //implement the draw function
    void menu_draw() override;

    //beyond what the base class stores, store some text to render as a header
    std::string header_text;
};