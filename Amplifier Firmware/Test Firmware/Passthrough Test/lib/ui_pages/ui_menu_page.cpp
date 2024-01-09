
#include <ui_menu_page.h>

UI_Menu::UI_Menu(size_t knob_led_index, UI_Page* _prev_page, UI_Menu_Item& _back_item):
    enc(encs[knob_led_index]), //save the rotary encoder
    led(leds[knob_led_index]), //save the LED
    to_prev_page(_prev_page) //initialize the page transition configured to go to the previous page
{
    //add the back item to our menu, and configure it as our back button
    add_menu_item(_back_item);
    set_back_item(_back_item);
}

//default constructor does nothing
UI_Menu::UI_Menu() {}

//setter methods to configure parameters not set in default constructor 
void UI_Menu::set_knob_led(size_t knob_led_index) {
    //save the encoder and led corresponding to the index provided
    enc = encs[knob_led_index];
    led = leds[knob_led_index];
}

void UI_Menu::set_prev_page(UI_Page* _prev_page) {
    //update the page transition to transition to the page passed in
    to_prev_page.set_to(_prev_page);
}

//all this function does is set the on-press callback of that item to run the page transition
void UI_Menu::set_back_item(UI_Menu_Item& back) {
    //and attach the `select()` callback function to execute the page transition back a page
    back.attach_on_select(to_prev_page);
}

void UI_Menu::add_menu_item(UI_Menu_Item& item) {
    //if this is our first menu item, save it as our default selected value
    if(last_item == nullptr) selected = &item;

    //add the passed in item after the last item in our list
    item.chain(last_item);

    //and set our new last item to be the item we just passed in and added to our menu
    last_item = &item;

    //also, grow our menu item size
    num_menu_items++;
}

//=================================== PRIVATE FUNCTIONS ==================================

//advance one position in our menu
void UI_Menu::next() {
    //sanity check --> don't move forward if we don't have anything ahead
    if(selected->get_next() == nullptr) return;

    //exit from the current selected item; call defocus
    //then get its next item and focus on it
    selected->defocus();
    selected = selected->get_next();
    selected->focus();

    //increment our position in our menu system
    menu_position++;
}

//go back one position in our menu
void UI_Menu::prev() {
    //sanity check --> don't move back if we don't have anything behind
    if(selected->get_prev() == nullptr) return;

    //exit from the current selected item; call defocus
    //then get its next item and focus on it
    selected->defocus();
    selected = selected->get_prev();
    selected->focus();

    //decrement our position in our menu system
    menu_position--;
}

//enter the menu
void UI_Menu::impl_on_entry() {
    //configure our encoder; just attach the on-click callback and configure limits (while also remembering position in menu)
    enc->attach_on_press(Context_Callback_Function<void>(reinterpret_cast<void*>(this), select_item_cb));
    enc->set_max_counts(num_menu_items, menu_position);

    //then call the specialized implementation of `on_entry()`
    menu_impl_on_entry();

    //finally, enter the menu at our selected menu item
    //NOTE: assuming `menu_pos` and effective position through menu are always coherent!
    selected->focus();
}

//exit the menu
void UI_Menu::impl_on_exit() {
    //exit the menu at our selected menu item
    //NOTE: assuming `menu_pos` and effective position through menu are always coherent!
    selected->defocus();

    //then call the specialize implementation of `on_exit()`
    menu_impl_on_exit(); //let the implementation decide whether we reset our position and selected item

    //finally detach our encoder callback function
    enc->attach_on_press({});
}

//manage the update of our ui_menu
void UI_Menu::draw() {
    //sample the encoder counts; move in the menu if we've changed spots
    uint32_t enc_counts = enc->get_counts();
    if(enc_counts > menu_position) next();
    else if(enc_counts < menu_position) prev();

    //and finally, call the implementation-specific draw
    menu_draw();
}

//call this function to select the present menu item
//context is expected to be a pointer to the `UI_Menu` instance, not the `UI_Menu_Item`!
void UI_Menu::select_item_cb(void* context) {
    UI_Menu* menu = reinterpret_cast<UI_Menu*>(context); //pull the instance outta the context

    //execute the `on_select()` callback for the selected item
    menu->selected->select();
}

//================================== DEFAULT IMPLEMENTATIONS OF VIRTUAL FUNCTIONS ====================================
//just set the LED according to the theme color (at the dim level)
void UI_Menu::menu_impl_on_entry() {
    led->set_color(menu_theme_color);
    led->set_brightness(App_Constants::UI_LED_LEVEL_DIM);
}

//and turn the LED off on the way out
void UI_Menu::menu_impl_on_exit() {
    led->set_color(RGB_LED::OFF);
}