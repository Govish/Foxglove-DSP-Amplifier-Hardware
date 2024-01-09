#include <ui_page_helpers/ui_menu_item_scroll.h>

//=============================== PUBLIC FUNCTIONS ===============================

//default constructor, just initialize the scrolling text
Menu_Item_Scroll::Menu_Item_Scroll():
    scroller()
{}

//constructor with parameter string--just pass this to the scrolling text
Menu_Item_Scroll::Menu_Item_Scroll(std::string _render_text):
    scroller(_render_text)
{}

//set the render text
void Menu_Item_Scroll::set_render_text(std::string _text_to_render) {
    scroller.set_render_text(_text_to_render);
}

u8g2_uint_t Menu_Item_Scroll::get_height(U8G2& graphics_handle) {
    //return menu item height based on the current font
    graphics_handle.setFontRefHeightAll();
    return (u8g2_uint_t)(graphics_handle.getAscent() - graphics_handle.getDescent());
}

//render the menu item
void Menu_Item_Scroll::draw(U8G2& graphics_handle, u8g2_uint_t x_left, u8g2_uint_t y_top, u8g2_uint_t x_right, u8g2_uint_t y_bot) {
    //set the bounding box of the text
    scroller.set_bounding_box(y_top, y_bot, x_left, x_right);

    //and render the scrolling text 
    scroller.render(graphics_handle);
}

//=============================== PRIVATE FUNCTIONS ===============================
//focusing and defocusing just sets the render text
void Menu_Item_Scroll::impl_focus() {
    scroller.start();
}

void Menu_Item_Scroll::impl_defocus() {
    scroller.stop();
}