#pragma once

#include <Arduino.h>
#include <U8g2lib.h>

#include <encoder.h>
#include <rgb.h>
#include <ui_page.h>
#include <utils.h>

class UI_Menu_Item; //forward declaring

//generic UI menu interface
//default implementation of `draw()` describes a whole-page menu
//can specialize this to render the display slightly differently
class UI_Menu : public UI_Page {
public:
    //save an encoder to use and the page corresponding to "back" select
    //pass in a pre-instantiated `back` item in whichever menu item style the user wants
    //allows for greater flexibility in the menu implementation
    //in the constructor, we'll basically reassign the `select` callback and save it to our class
    //      \--> won't touch any other callbacks, so can leverage the `on_focus()` or `on_defocus()` as necessary
    //also take in a theme color for the menu to 
    UI_Menu(size_t knob_led_index, UI_Page* _prev_page, UI_Menu_Item& _back_item);

    //also provide an default constructor that initializes pointers to null
    //makes initializing arrays of menus slightly easier
    UI_Menu();

    //virtual destructor to ensure any child pages are also called
    virtual ~UI_Menu() {}

    //provide ways of attaching/configuring variables not found in the default constructor
    void set_knob_led(size_t knob_led_index);
    void set_prev_page(UI_Page* _prev_page);
    void set_back_item(UI_Menu_Item& back);

    //set the theme color of the menu
    //default implementation uses this to light the LED on entry
    inline void set_theme_color(RGB_LED::COLOR _theme_color) { menu_theme_color = _theme_color; }

    //function to add items to the current menu
    //NOTE: items must have permanent storage duration! They aren't copied by this function, just pointed to!
    void add_menu_item(UI_Menu_Item& item);

protected:
    //even further specializations of `draw()`, `on_entry()` and `on_exit()`
    //default implementation of `entry` and `exit` just set the LED
    //default implementation doesn't do any rendering --> leave these to the children
    virtual void menu_impl_on_entry();
    virtual void menu_impl_on_exit();
    virtual void menu_draw() {}

    //store a pointer to the rotary encoder and LED screen will work with
    //additinally store a theme color for the menu --> light the LED this color by default on menu entry
    Rotary_Encoder* enc;
    RGB_LED* led;
    RGB_LED::COLOR menu_theme_color;

    //have a pointer to the selected menu item
    UI_Menu_Item* selected;

    //have some primitives that keep track of how many menu items there are
    //and where in the list we are
    uint32_t num_menu_items = 1; //start with just the back button
    uint32_t menu_position = 0; //maintain an index variable of where we are in the menu

    //also own a `UI_Menu_Item` corresponding to the last menu item in our list
    UI_Menu_Item* last_item; //just a convenience pointer with `add_menu_item()`

    //own a transition that goes to the previous page
    Pg_Transition to_prev_page;

private:
    //call this function to select the currently focused item
    //NOTE: will just execute the `on_select()` callback! any page transitions must be attached to `on_select`!
    static void select_item_cb(void* context);

    //use these functions to configure and release our encoder along with managing menu traversal
    void impl_on_entry() override;
    void impl_on_exit() override;
    void draw() override;

    //use these functions to move through the list of menu items
    void next();
    void prev();
};


//interface for a menu item --> every menu item must implement this!
class UI_Menu_Item {
public:
    //default constructor does saves the previous menu item and updates the previous menu item
    UI_Menu_Item() {}

    //virtual destructor to forward to children
    virtual ~UI_Menu_Item() {}

    //function to chain menu items together
    //the current item's previous item is the item passed in
    //the item passed in's next item is this current instance
    inline void chain(UI_Menu_Item* _prev) { prev = _prev; if(prev != nullptr) prev->next = this; }
    
    //hooks to attach callback functions for focus, defocus or select events
    inline void attach_on_select(Context_Callback_Function<void> _cb) { on_select_cb = _cb; }

    //functions to run the appropriate callback when the menu item is focused or selected
    //intended to be called by the UI_Menu
    inline void focus() { impl_focus(); }
    inline void defocus() { impl_defocus(); }
    inline void select() { on_select_cb(); }

    //get the next and previous menu items
    inline UI_Menu_Item* get_next() { return next; }
    inline UI_Menu_Item* get_prev() { return prev; }

    //actually render the menu item, pass in a graphics handle 
    //x_loc and y_loc describe the top left corner of said menu item
    //implemented by children 
    virtual void draw(U8G2& graphics_handle, u8g2_uint_t x_left, u8g2_uint_t y_top, u8g2_uint_t x_right, u8g2_uint_t y_bot) {}

    //primitive to get the menu item height
    //implemented by children
    virtual u8g2_uint_t get_height(U8G2& graphics_handle) { return 0; }

protected:
    //declare functions that determine how the particular menu item implements focus and defocus
    //e.g. for scrolling menu items, this could just be starting/stopping scrolling 
    virtual void impl_focus() {}
    virtual void impl_defocus() {}

    //save callback functions to execute on focusing, defocusing or selection
    Context_Callback_Function<void> on_focus_cb;
    Context_Callback_Function<void> on_defocus_cb;
    Context_Callback_Function<void> on_select_cb;

    //hold pointers to previous menu items and next menu items
    UI_Menu_Item* prev = nullptr;
    UI_Menu_Item* next = nullptr;
};