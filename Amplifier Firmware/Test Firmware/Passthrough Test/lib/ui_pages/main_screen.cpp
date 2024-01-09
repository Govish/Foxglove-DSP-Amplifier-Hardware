
#include <main_screen.h>

//============================ STATIC MEMBER DEFINITIONS ========================

const std::array<uint8_t, (Main_Screen::SETTINGS_ICON_WIDTH+7)/8 * Main_Screen::SETTINGS_ICON_HEIGHT> 
Main_Screen::settings_icon = {
    0x7C, 0x00, 0x00, 0x6C, 0x00, 0x00, 0xEF, 0xFF, 0x7F, 0x6C, 0x00, 0x00, 
    0x7C, 0x00, 0x3E, 0x00, 0x00, 0x36, 0xFF, 0xFF, 0x77, 0x00, 0x00, 0x36, 
    0x80, 0x0F, 0x3E, 0x80, 0x0D, 0x00, 0xFF, 0xFD, 0x7F, 0x80, 0x0D, 0x00, 
    0x80, 0x0F, 0x00
};

//================================================ PUBLIC FUNCTIONS ============================================

Main_Screen::Main_Screen(   std::array<std::unique_ptr<Effect_Interface>, App_Constants::NUM_EFFECTS>& _active_effects,
                            std::array<UI_Page*, App_Constants::NUM_EFFECTS>& effects_sel_pages,
                            UI_Page* settings_page ):
    Main_Screen(_active_effects) //forward argument to the other constructor
{
    //copy our effect select pages into our `main_select_pages`
    std::copy(effects_sel_pages.begin(), effects_sel_pages.end(), main_select_pages.begin());

    //and copy our settings page into the right spot
    main_select_pages[App_Constants::NUM_EFFECTS] = settings_page;
}

//alternate constructor
Main_Screen::Main_Screen(std::array<std::unique_ptr<Effect_Interface>, App_Constants::NUM_EFFECTS>& _active_effects) {
    //no subarray of std::array so have to do this gross thing:
    //create local vars of pointers that are the right length and copy stuff into them
    std::array<RGB_LED*, App_Constants::NUM_EFFECTS> fx_leds;
    std::array<Rotary_Encoder*, App_Constants::NUM_EFFECTS> fx_encs;
    std::copy(leds.begin(), leds.begin() + App_Constants::NUM_EFFECTS, fx_leds.begin()); //copy only a certain amount of pointers over
    std::copy(encs.begin(), encs.begin() + App_Constants::NUM_EFFECTS, fx_encs.begin());

    //then only can we initialize our `erc` array
    ercs = Effect_Resource_Collection::mk_ercs(this, _active_effects, fx_leds, fx_encs);

    //additionally, configure our main menu text
    //we'll configure the "nothing selected" text in our entry function
    //since that requires knowing what effects are active
    for(size_t i = 0; i < App_Constants::NUM_EFFECTS; i++) {
        render_texts[i].set_render_text("Channel [" + std::to_string(i + 1) + "] - Choose Effect");
    }
    render_texts[App_Constants::NUM_EFFECTS].set_render_text("Settings");
}

//provide a function to attach effects select pages
void Main_Screen::attach_effects_sel(std::array<UI_Page*, App_Constants::NUM_EFFECTS>& effects_sel_pages) {
    //copy our effect select pages into our `main_select_pages`
    std::copy(effects_sel_pages.begin(), effects_sel_pages.end(), main_select_pages.begin());
}

//provide a function to attach a settings page
void Main_Screen::attach_settings(UI_Page* settings_page) {
    //and copy our settings page into the right spot
    main_select_pages[App_Constants::NUM_EFFECTS] = settings_page;
}

//======================================== OVERRIDEN DERIVED CLASS FUNCTIONS ===================================

void Main_Screen::impl_on_entry() {
    //for each of our effects channels
    for(Effect_Resource_Collection& erc : ercs) {
        //internally update the edit destination pages and the quick edit params as necessary
        erc.update_from_effect();

        //if we've enabled quick editing, and the quick edit param exists (not nullptr)
        if(App_Constants::QUICK_EDIT_ENABLED && erc.quick_edit_param != nullptr) {
            //set the `on_change()` callback function to navigate us to the quick edit page
            //the quick edit page will take care of actually adjusting and visualizing the parameter
            erc.enc->attach_on_change(erc.to_quick_edit);
        }

        //set the return page of the effect to this page
        erc.effect->get()->set_return_page(this);

        //attach the `on_press()` interrupt to navigate us to the effect edit menu
        erc.enc->attach_on_press(erc.to_effect_edit);

        //set the corresponding LED to the theme color of the effect at a dim level
        erc.led->set_color(erc.effect->get()->get_theme_color());
        erc.led->set_brightness(App_Constants::UI_LED_LEVEL_DIM);
    }

    //for the main encoder (last encoder in the array)
    //set the max number of counts to the number of options available
    // `on_change()` will update an index variable and the page transition
    // `on_press()` executes the page transition
    Rotary_Encoder* main_enc = encs.back();
    main_enc->set_max_counts(App_Constants::NUM_EFFECTS + 1, main_selected_item);
    main_enc->attach_on_change( Context_Callback_Function<void>(reinterpret_cast<void*>(this), main_change_cb ));
    main_enc->attach_on_press( main_select_transition );

    //configure our "nothing selected" text
    //list the names of the effects currently active
    //and save this string to our last render text index
    std::string effect_names_text = "Active Effects:";
    for(size_t i = 0; i < App_Constants::NUM_EFFECTS; i++) {
        effect_names_text += " [" + std::to_string(i + 1) + "] ";
        effect_names_text += ercs[i].effect->get()->get_name();
    }
    render_texts.back().set_render_text(effect_names_text);

    //and trigger our scrolling text in advance of entering our menu
    last_selected_item = main_selected_item;
    render_texts[main_selected_item].start();
}

void Main_Screen::impl_on_exit() {
    //stop rendering the particular text on the screen
    render_texts[main_selected_item].stop();
    
    //turn off all of our LEDs
    for(RGB_LED* led : leds)
        led->set_color(RGB_LED::OFF);

    //detach our encoder callback functions
    for(Rotary_Encoder* enc : encs) {
        //using default constructor for context callback function
        //which basically sets it to nullptr --> doesn't redirect
        enc->attach_on_change({});
        enc->attach_on_press({});
    }
}

//called every `SCREEN_REDRAW_MS`
void Main_Screen::draw() {

    //###################### RENDERING #####################

    //Start render process by clearing the draw buffer
    graphics_handle.clearBuffer();

    //######################## SIGNAL "PIPE" RENDERING #####################
    //simple box to draw a "signal input and output" shape behind all the icons
    //shape will consist of a rectangular frame at the centerline of the icons, spanning across the width of the screen
    //then a triangle designating the input and output

    //draw the input triangle
    static const u8g2_uint_t intri_height = 9;
    static const u8g2_uint_t intri_width = 4;
    static const u8g2_uint_t intri_x_start = 0;
    static const u8g2_uint_t intri_x_end = intri_x_start + intri_width;
    static const u8g2_uint_t intri_y_mid = graphics_handle.getHeight() - App_Constants::EFFECT_PADDING - ((App_Constants::EFFECT_ICON_HEIGHT + 1) >> 1);
    static const u8g2_uint_t intri_y_start = intri_y_mid - ((intri_height + 1) >> 1);
    static const u8g2_uint_t intri_y_end = intri_y_mid + ((intri_height + 1) >> 1);
    graphics_handle.drawTriangle(   intri_x_start, intri_y_start,
                                    intri_x_start, intri_y_end,
                                    intri_x_end, intri_y_mid);

    //draw the output triangle
    static const u8g2_uint_t outtri_height = intri_height;
    static const u8g2_uint_t outtri_width = intri_width;
    static const u8g2_uint_t outtri_x_start = graphics_handle.getWidth() - outtri_width - 1;
    static const u8g2_uint_t outtri_x_end = graphics_handle.getWidth();
    static const u8g2_uint_t outtri_y_mid = intri_y_mid;
    static const u8g2_uint_t outtri_y_start = intri_y_start;
    static const u8g2_uint_t outtri_y_end = intri_y_end;
    graphics_handle.drawTriangle(   outtri_x_start, outtri_y_start,
                                    outtri_x_start, outtri_y_end,
                                    outtri_x_end, outtri_y_mid);

    //draw the rectangle going across the screen
    static const u8g2_uint_t rect_width = graphics_handle.getWidth() - outtri_width;
    static const u8g2_uint_t rect_height = 3;
    static const u8g2_uint_t rect_x_start = 0;
    static const u8g2_uint_t rect_y_start = graphics_handle.getHeight() - App_Constants::EFFECT_PADDING - ((App_Constants::EFFECT_ICON_HEIGHT + rect_height) >> 1);
    graphics_handle.drawBox(rect_x_start, rect_y_start, rect_width, rect_height);


    //######################## EFFECT ICON RENDERING #####################

    //RENDER THE EFFECT ICONS --> start by computing some dimension constants
    static const u8g2_uint_t total_icon_width = App_Constants::NUM_EFFECTS * 
                                                (App_Constants::EFFECT_ICON_WIDTH + App_Constants::EFFECT_PADDING) - 
                                                App_Constants::EFFECT_PADDING;
    static const u8g2_uint_t icon_start_x = (graphics_handle.getWidth() - total_icon_width) >> 1;
    static const u8g2_uint_t icon_start_y = (graphics_handle.getHeight() - App_Constants::EFFECT_ICON_HEIGHT - App_Constants::EFFECT_PADDING);

    //now actually draw all the icons on the screen
    u8g2_uint_t x_coord = icon_start_x;
    for(auto& erc : ercs) {
        graphics_handle.drawXBMP(   x_coord, icon_start_y, 
                                    App_Constants::EFFECT_ICON_WIDTH, App_Constants::EFFECT_ICON_HEIGHT, erc.effect->get()->get_icon().data());
        x_coord += App_Constants::EFFECT_ICON_WIDTH + App_Constants::EFFECT_PADDING;
    }

    //########################## SETTINGS ICON RENDERING #######################

    //render an icon in the top right corner as "settings"
    static u8g2_uint_t settings_x_start = graphics_handle.getWidth() - SETTINGS_ICON_WIDTH - SETTINGS_ICON_X_PADDING;
    static u8g2_uint_t settings_y_start = SETTINGS_ICON_Y_PADDING;
    graphics_handle.drawXBMP(   settings_x_start, settings_y_start,
                                SETTINGS_ICON_WIDTH, SETTINGS_ICON_HEIGHT,
                                settings_icon.data());

    //###################### end BACKGROUND RENDERING begin MAIN ENCODER HANDLING #####################

    //depending on what the main knob is selecting, render the screen and set the LED appropriately
    if(main_selected_item < App_Constants::NUM_EFFECTS) { //we're selecting an effect
        //set the main LED (the last one) color to match the effect theme color
        leds.back()->set_color(ercs[main_selected_item].effect->get()->get_theme_color());
        leds.back()->set_brightness(App_Constants::UI_LED_LEVEL_DIM);

        //draw a graphic rectangle around the icon of the selected effect, inverting its colors
        //do so by computing some parameters related to the rectangle
        static const u8g2_uint_t icon_expand = (App_Constants::EFFECT_PADDING + 1) >> 1;
        u8g2_uint_t box_x = (icon_start_x - icon_expand) + (App_Constants::EFFECT_ICON_WIDTH + App_Constants::EFFECT_PADDING) * main_selected_item; 
        static const u8g2_uint_t box_y = icon_start_y - icon_expand;
        static const u8g2_uint_t box_width = App_Constants::EFFECT_ICON_WIDTH + (icon_expand << 1);
        static const u8g2_uint_t box_height = App_Constants::EFFECT_ICON_HEIGHT + (icon_expand << 1);
        static const u8g2_uint_t box_r = 1;

        //actually draw the rectangle
        graphics_handle.setDrawColor(2); //change mode such that it inverts the icon color
        graphics_handle.drawRBox(box_x, box_y, box_width, box_height, box_r);
        graphics_handle.setDrawColor(1); //revert the graphics mode to default

    }

    else if(main_selected_item == App_Constants::NUM_EFFECTS) { //we're selecting our settings menu
        //compute the indices into the theme colors array for which to fade
        size_t blend_from_index = (size_t)blend_counter; //will be a valid index
        size_t blend_to_index = (blend_from_index + 1) % App_Constants::SPLASH_LED_COLORS.size(); //wrap around as necessary
        float throwaway; //need this to run modf
        float color_blend_ratio = modf(blend_counter, &throwaway); // value from 0 -> 1 on how much we need to
        __unused(throwaway); //not using the integer part

        //compute and write the blended color at low brightness
        RGB_LED::COLOR blend_from = App_Constants::SPLASH_LED_COLORS[blend_from_index];
        RGB_LED::COLOR blend_to = App_Constants::SPLASH_LED_COLORS[blend_to_index];
        RGB_LED::COLOR blend_color = RGB_LED::COLOR::blend(blend_from, blend_to, color_blend_ratio);
        leds.back()->set_color(blend_color, App_Constants::UI_LED_LEVEL_DIM);

        //increment and wrap the counter
        blend_counter += App_Constants::SETTINGS_LED_FADE_INC;
        if(blend_counter > App_Constants::SPLASH_LED_COLORS.size()) blend_counter -= (float)App_Constants::SPLASH_LED_COLORS.size();


        //draw a graphic rectangle around the icon of the settings menu, inverting its colors
        //do so by computing some parameters related to the rectangle
        static const u8g2_uint_t settings_expand_x = (SETTINGS_ICON_X_PADDING + 1) >> 1;
        static const u8g2_uint_t settings_expand_y = (SETTINGS_ICON_Y_PADDING + 1) >> 1;
        static const u8g2_uint_t setbox_x = settings_x_start - settings_expand_x; 
        static const u8g2_uint_t setbox_y = settings_y_start - settings_expand_y;
        static const u8g2_uint_t setbox_width = SETTINGS_ICON_WIDTH + (settings_expand_x << 1);
        static const u8g2_uint_t setbox_height = SETTINGS_ICON_HEIGHT + (settings_expand_y << 1);
        static const u8g2_uint_t setbox_r = 0;

        //actually draw the rectangle
        graphics_handle.setDrawColor(2); //change mode such that it inverts the icon color
        graphics_handle.drawRBox(setbox_x, setbox_y, setbox_width, setbox_height, setbox_r);
        graphics_handle.setDrawColor(1); //revert the graphics mode to default
    }

    else //not selecting anything with main
        //turn the LED off, don't draw anything
        leds.back()->set_color(RGB_LED::OFF);


    //############ WRITE A SCROLLING DISPLAY STRING IN THE TOP LEFT CORNER ###############
    //start/stop text rendering when we change menu items
    if(last_selected_item != main_selected_item) {
        render_texts[last_selected_item].stop();
        render_texts[main_selected_item].start();
        last_selected_item = main_selected_item;
    }

    //set the bounding box for the active text box
    auto& render_text = render_texts[main_selected_item];
    render_text.set_bounding_box(   0, SETTINGS_ICON_HEIGHT + (SETTINGS_ICON_Y_PADDING << 1),
                                    0, graphics_handle.getWidth() - (SETTINGS_ICON_WIDTH + (SETTINGS_ICON_X_PADDING << 1)) );
    render_text.render(graphics_handle);

    //###################### SEND DISPLAY BUFFER #####################

    //write the buffer out to the OLED
    graphics_handle.sendBuffer();
}

//====================================== PRIVATE FUNCTIONS ====================================

//callback function when the main knob (final knob in array) is being rotated
void Main_Screen::main_change_cb(void* context) {
    //get the pointer to the actual screen instance
    Main_Screen* s = reinterpret_cast<Main_Screen*>(context);

    //read the counts of the main encoder (last encoder in the array)
    //that corresponds to the selected item
    s->main_selected_item = s->encs.back()->get_counts();

    //and set the transition to the appropriate page (based on the selected item)
    s->main_select_transition.set_to(s->main_select_pages[s->main_selected_item]);
}