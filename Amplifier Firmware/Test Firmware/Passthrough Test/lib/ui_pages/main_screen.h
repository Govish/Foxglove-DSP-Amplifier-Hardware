#pragma once

#include <memory> //for unique_ptr
#include <Arduino.h>

#include <ui_page.h> //inherit from here
#include <idle_screen.h> //for noise reduction when no user inputs are active
#include <quick_edit_screen.h> //own a couple of these to implement quick editing of parameters
#include <effect_interface.h> //leverage the effect interface to modify effects
#include <effect_param.h> //leverage parameter interface to hold quick edit params
#include <ui_page_helpers/scroll_string.h> //for displaying scrolling text
#include <encoder.h>
#include <rgb.h>

class Main_Screen : public UI_Page {

public:
    //Main screen wants unique pointers to active effects
    //      \--> specifically using `unique_ptr`s because the intention is to heap-allocate audio effects
    //      \--> this is the safest way to manage heap memory to ensure no memory leaks and dangling pointers
    //additionally, pass in (array of) pointers to effects selection pages and settings page
    //      \--> these will be created and initialized externally
    Main_Screen(    std::array<std::unique_ptr<Effect_Interface>, App_Constants::NUM_EFFECTS>& _active_effects,
                    std::array<UI_Page*, App_Constants::NUM_EFFECTS>& effects_sel_pages,
                    UI_Page* settings_page );
    
    //alternative constructor where just the active effects are stored
    Main_Screen(std::array<std::unique_ptr<Effect_Interface>, App_Constants::NUM_EFFECTS>& _active_effects);

    //provide functions to attach the effects select pages and the settings pages
    void attach_effects_sel(std::array<UI_Page*, App_Constants::NUM_EFFECTS>& effects_sel_pages);
    void attach_settings(UI_Page* settings_page);


private:
    //override all the `UI_page()` functions
    void draw() override; //gets called regularly at `App_Constants::SCREEN_REDRAW_MS`
    void impl_on_entry() override; //override entry function --> set up all the LEDs and encoders
    void impl_on_exit() override; //override exit function --> turn off all LEDs, detach encoders

    //callback function when the main knob (final knob in array) is being rotated
    static void main_change_cb(void* context);

    //create a structure that collects everything related to an effects channel as relevant to the UI
    //includes:
    //  - a pointer to the particular `Main_Page` instance to reference instance parameters
    //  - a pointer to a `std::unique_ptr<Effect_Interface>` that references the present active effect
    //      \--> useful to have a direct pointer to this for rendering functions
    //      \--> pointer to pointer is weird but forced to do it to have a default constructor       
    //  - page transitions to each edit each effect's parameters (`effect`s are also UI pages)
    //  - pointer to the quick edit parameter (to easily check if it's not null)
    //  - A screen to quick edit a parameter
    //  - A transition to go between the main page and the quick edit page
    //  - pointer to an LED to illuminate
    //  - pointer to an encoder to hook up callback functions to
    struct Effect_Resource_Collection {
        Main_Screen* instance; //main page instance
        /* Entire effect-related fields */
        std::unique_ptr<Effect_Interface>* effect; 
        Pg_Transition to_effect_edit;
        /* Quick-edit parameter related fields */
        Effect_Parameter* quick_edit_param;
        Quick_Edit_Screen quick_edit_page;
        Pg_Transition to_quick_edit;
        /* Hardware related fields  */
        RGB_LED* led;
        Rotary_Encoder* enc;

        //implement a pseudo-constructor --> have to implement a default constructor if we want an array of these
        //technically possible to work witha non-default constructuro, but REALLY gross to implement --> this is the lesser of two evils 
        void configure(
            Main_Screen* _instance,
            std::unique_ptr<Effect_Interface>* _effect,
            RGB_LED* _led,
            Rotary_Encoder* _enc,
            const size_t index) 
        {
            this->instance = _instance;                 //main page instance     
            this->effect = _effect;                     //pointer to a pointer is gross, but have to do it this way
            this->to_effect_edit.set_to(effect->get());
            this->quick_edit_param = effect->get()->get_quick_edit_param(); //get the default quick edit parameter
            this->quick_edit_page.set_main_pg_index(_instance, index);  //retroactively fill in details about the quick edit page
            this->to_quick_edit.set_to(&quick_edit_page);
            this->led = _led;   //save the LED to use
            this->enc = _enc;   //save the encoder to use
        
            //set the quick edit parameter and the theme color to use in the quick edit page
            //use the default quick edit parameter the effect initializes with
            //looks pretty gross since we got a pointer to a pointer (necessary evil)
            this->quick_edit_page.set_qe_param_color(   this->quick_edit_param, 
                                                        effect->get()->get_theme_color());
        }

        //implement a way to update the struct after the effect gets updated
        //MUST call this before using the effect edit page and the quick edit page!
        //makes sense to call this in `impl_on_entry()`
        void update_from_effect() {
            //update the destination pointer in the page transition
            //useful in case location of effect in the heap has changed
            this->to_effect_edit.set_to(this->effect->get());

            //update the retrieved quick edit parameter
            this->quick_edit_param = this->effect->get()->get_quick_edit_param();

            //update the quick edit page with the new parameter and theme color as necessary
            //will ensure quick edit param is valid and updated, and theme color is updated
            this->quick_edit_page.set_qe_param_color(   this->quick_edit_param, 
                                                        effect->get()->get_theme_color());
        }

        //and a function to make an array of these for convenience
        template<size_t N>
        static constexpr std::array<Effect_Resource_Collection, N> mk_ercs(
            Main_Screen* inst,
            std::array<std::unique_ptr<Effect_Interface>, N>& effects,
            std::array<RGB_LED*, N>& leds,
            std::array<Rotary_Encoder*, N>& encs)
        {
            //start with default initializing all of our `Effect_Resource_Collection`s
            std::array<Effect_Resource_Collection, N> ercs{};

            //and now properly configure each one of them
            for (size_t i = 0; i < N; i++) {
                Effect_Resource_Collection& erc = ercs[i];
                erc.configure(inst, &effects[i], leds[i], encs[i], i);
            }

            //return the array of them
            return ercs;
        }
    };

    //own an array of `Effect_Resource_Collection`s
    //really cleans up the code and allows it to be a bit more scalable
    //each effect channel has all the resources it needs in one single struct
    //can basically initialize this element-wise 
    std::array<Effect_Resource_Collection, App_Constants::NUM_EFFECTS> ercs;

    //transition to an idle screen for effect noise reduction
    Idle_Screen idle_screen;
    Pg_Transition to_idle_screen;
    Scheduler idle_screen_timeout;

    //for the main encoder, hold an index value of its position
    //this will control which item is selected (effect channels, settings, or nothing)
    //screen drawing and LED updating will be performed accordingly
    // 0 - NUM_EFFECTS  --> particular effect channel is selected
    // NUM_EFFECTS      --> settings are selected
    // NUM_EFFECTS + 1  --> nothing is selected
    uint32_t main_selected_item = App_Constants::NUM_EFFECTS + 1; //start with nothing being selected
    uint32_t last_selected_item = 0;

    //these will be the pages we'll navigate to depending on the value of `main_selected_item`
    //the first `NUM_EFFECTS` elements will hold pointers effect selection pages
    //the one after that will hold a pointer to the SETTINGS page
    //and the last one will be a nullpointer, so we don't actually do anything
    std::array<UI_Page*, App_Constants::NUM_EFFECTS + 2> main_select_pages = {nullptr};

    //have a transition that actually moves between the pages
    Pg_Transition main_select_transition;

    //we'll render an icon for the settings menu --> maintain a small bitmap along with its dimensions
    static constexpr size_t SETTINGS_ICON_WIDTH = 23;
    static constexpr size_t SETTINGS_ICON_HEIGHT = 13;
    static constexpr u8g2_uint_t SETTINGS_ICON_X_PADDING = 2;
    static constexpr u8g2_uint_t SETTINGS_ICON_Y_PADDING = 1;
    static const std::array<uint8_t, (SETTINGS_ICON_WIDTH+7)/8 * SETTINGS_ICON_HEIGHT> settings_icon;

    //We'll need to also render some text in the top left corner depending on the settings menu position
    std::array<Scroll_String, App_Constants::NUM_EFFECTS + 2> render_texts;

    //and finally a little primitive to keep track of where we are when blending colors
    //animating the LED when hovering over the settings menu
    float blend_counter = 0;
};