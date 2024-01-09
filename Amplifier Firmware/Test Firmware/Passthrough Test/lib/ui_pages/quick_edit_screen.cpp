
#include <quick_edit_screen.h>

Quick_Edit_Screen::Quick_Edit_Screen():
    back_to_main(),     //default page transition
    enc_led_index(0)    //default index
{}

Quick_Edit_Screen::Quick_Edit_Screen(UI_Page* _main_page, const size_t _enc_led_index):
    back_to_main(_main_page), //configure the page transition that returns to the main page
    enc_led_index(_enc_led_index) //and save the index of the encoder/LED
{}

void Quick_Edit_Screen::set_main_pg_index(UI_Page* _main_page, const size_t _enc_led_index) {
    //update our page transition 
    back_to_main.set_to(_main_page);

    //and update our index re: which encoder and LED to use
    enc_led_index = _enc_led_index;
}

//set the particular quick edit parameter of interest and its theme color
void Quick_Edit_Screen::set_qe_param_color(Effect_Parameter* _qe_param, RGB_LED::COLOR _theme_color) {
    qe_param = _qe_param;
    theme_color = _theme_color;
}

//========================================== PRIVATE FUNCTION DEFS =======================================

//refresh the transition timeout
void Quick_Edit_Screen::refresh_edit_timeout(void* context) {
    //get the instance from the context
    Quick_Edit_Screen& qe_screen = *reinterpret_cast<Quick_Edit_Screen*>(context);

    //refresh the timeout by rescheduling the transition
    qe_screen.done_editing_sched.schedule_oneshot_ms(qe_screen.back_to_main, App_Constants::QUICK_EDIT_TIMEOUT_MS);
}

//override the standard UI page functions
void Quick_Edit_Screen::impl_on_entry() {
    //turn off all LEDs
    for(RGB_LED* led : leds)
        led->set_color(RGB_LED::OFF);

    //if we actually have a quick edit parameter
    if(qe_param != nullptr) {
        //light the LED at the particular index brightly
        leds[enc_led_index]->set_color(theme_color);
        leds[enc_led_index]->set_brightness(App_Constants::UI_LED_LEVEL_BRIGHT);

        //attach the encoder to the particular parameter
        qe_param->attach_configure_enc(*encs[enc_led_index]);

        //configure the encoder's on change interrupt to refresh the timeout
        //to return to the homescreen
        encs[enc_led_index]->attach_on_change(Context_Callback_Function<void>(reinterpret_cast<void*>(this), refresh_edit_timeout)); 
    }

    //and of course, schedule the transition back to the home screen
    done_editing_sched.schedule_oneshot_ms(back_to_main, App_Constants::QUICK_EDIT_TIMEOUT_MS);
}

void Quick_Edit_Screen::impl_on_exit() {
    //don't need to do anything if our quick edit param didn't exist
    if(qe_param == nullptr) return;

    //turn off our LED
    leds[enc_led_index]->set_color(RGB_LED::OFF);

    //detach our encoder and remove its callback
    qe_param->detach_enc();
    encs[enc_led_index]->attach_on_change({});
}

void Quick_Edit_Screen::draw() {
    if(qe_param == nullptr) {
        /* TODO, print something like "no quick edit parameter"*/
    }
    else {
        /* TODO */
    }
}
