
#include <ui_system.h>

#include <array>
#include <utility> //for std::pair

#include <all_effects.h> //class that maintains active effects in the system

//======= UI page includes ========
#include <splash_screen.h>
#include <quick_edit_screen.h>
#include <menu_full_screen.h>
#include <main_screen.h>

//======= UI helper includes =======
#include <ui_page_helpers/ui_menu_item_scroll.h>

//======================== STATIC VARIABLE DEFINITION =======================

//some pointers to UI pages 
UI_Page* UI_System::entry = nullptr; //start this off as a nullptr
UI_Page* UI_System::app_main_screen = nullptr; //start this off as a nullptr too

//========================== PUBLIC FUNCTION DEFS ========================

//really just this public function --> make the UI system
//all pages will be created statically, so they'll persist after this function call
void UI_System::make_ui() {
    //only run this code once --> create and initilize all the UI pages
    static bool initialized = false;
    if(initialized) return;


    //make our main UI page, active effects will be maintained by `Effects_Manager`
    //save this main screen so we can return to it from callback functions
    static Main_Screen main_screen(Effects_Manager::get_active_effects());
    app_main_screen = &main_screen;


    //make and configure an array of effect select pages
    static std::array<Menu_Full_Screen, App_Constants::NUM_EFFECTS> effect_sel_pages;
    static std::array<UI_Page*, effect_sel_pages.size()> effect_sel_page_ptrs; //array of pointers we'll need later

    for(size_t effect_index = 0; effect_index < effect_sel_pages.size(); effect_index++) {
        auto& sel_page = effect_sel_pages[effect_index]; //get the effect select page at the particular index
        effect_sel_page_ptrs[effect_index] = &sel_page; //configure pointer

        //configure the particular select page itself
        sel_page.set_prev_page(&main_screen);   //sets where our `back` function navigates us
        sel_page.set_knob_led(App_Constants::NUM_EFFECTS);                  //use the main knob and LED
        sel_page.set_theme_color(App_Constants::SPLASH_LED_COLORS[0]);      //nothing fancy for our settings page LED color as of now
        sel_page.set_header_text("Choose Effect " + std::to_string(effect_index + 1)); //make a string for the particular channel

        //NOTE: the section below here is kinda gross with heap allocation and just general code implementation
        //I'm doing stuff this way mostly due to limitations of the language 
        //  \--> (std::array needs a constexpr argument for its size --> apparently can't determine # of effects at compile time?
        //All things considered, this isn't the *worst* thing in the world, but heap allocation always feels a bit gross on embedded
        //but this function shouldn't create any memory leaks or performance bottlenecks, since it only runs once at the beginning of the program

        //create a `back` item on the heap; I guess this is fine since we'll never need to destroy this in the lifetime of the code
        //this code will also only run once so shouldn't be a memory leak
        Menu_Item_Scroll* back_item = new Menu_Item_Scroll("<< Back");
        
        //set the back button as the back item and add this as a menu item
        sel_page.set_back_item(*back_item);
        sel_page.add_menu_item(*back_item);
        
        //make menu items for each name
        //each menu item on select will redirect to a forwarding function
        //that unpacks a pair of [effect_index, new_effect_no] and forwards that to `replace(...)`
        auto all_effect_names = Effects_Manager::get_available_names(); //grab all the effect names
        for(size_t effect_no = 0; effect_no < all_effect_names.size(); effect_no++) {
            //get the effect name corresponding to the effect number
            const auto& effect_name = all_effect_names[effect_no];

            //create a new scrolling menu item on the heap based off that effect name
            Menu_Item_Scroll* item_choose_effect = new Menu_Item_Scroll(effect_name);

            //make a pair on the heap corresponding to [first=] effect_index, [second=] new_effect_number
            std::pair<size_t, size_t>* effect_index_new_no = new std::pair<size_t, size_t>;
            effect_index_new_no->first = effect_index;
            effect_index_new_no->second = effect_no;

            //set the `on_select()` callback function the internal forwarding function
            //and pass the heap-allocated pair we just created as context
            item_choose_effect->attach_on_select(   Context_Callback_Function<void>(reinterpret_cast<void*>(effect_index_new_no),
                                                    replace_effect_cb));

            //attach this item to our particular select page
            sel_page.add_menu_item(*item_choose_effect);
        }
    }


    //make and configure a settings page
    static Menu_Full_Screen settings_page;
    settings_page.set_prev_page(&main_screen); //return to the main screen
    settings_page.set_knob_led(App_Constants::NUM_ENCODERS - 1); //use the last encoder + LED
    settings_page.set_theme_color(App_Constants::SPLASH_LED_COLORS[0]); //nothing fancy for our settings page LED color as of now
    settings_page.set_header_text("Settings");
    
    /* TODO: populate our settings page with menu items, including BACK */
    static Menu_Item_Scroll settings_back("<< Back");
    static Menu_Item_Scroll settings_1("Dummy Setting 1 - Sample Text");
    static Menu_Item_Scroll settings_2("Dummy Setting 2 - Sample Text");
    static Menu_Item_Scroll settings_3("Dummy Setting 3 - Sample Text");
    static Menu_Item_Scroll settings_4("Dummy Setting 4 - Sample Text");
    settings_page.add_menu_item(settings_back);
    settings_page.add_menu_item(settings_1);
    settings_page.add_menu_item(settings_2);
    settings_page.add_menu_item(settings_3);
    settings_page.add_menu_item(settings_4);
    settings_page.set_back_item(settings_back);


    //attach our effects select pages and settings pages to our main page
    main_screen.attach_effects_sel(effect_sel_page_ptrs);
    main_screen.attach_settings(&settings_page);


    //create a splash screen, and conenct it to the main screen
    //THIS IS OUR ENTRY POINT --> save this to our pointer
    static Splash_Screen splash_screen(&main_screen);
    entry = &splash_screen;
}

//forwarding function to choose a new effect for a particular channel
void UI_System::replace_effect_cb(void* context) {
    //interpret the context as a pointer to a pair
    std::pair<size_t, size_t>* effect_index_new_no = reinterpret_cast<std::pair<size_t, size_t>*>(context);

    //forward the function call to the `replace(...)` function
    Effects_Manager::replace(effect_index_new_no->first, effect_index_new_no->second);

    //quickly create a page transition back to the main screen and execute it
    Pg_Transition back_to_main(app_main_screen);
    back_to_main();
}