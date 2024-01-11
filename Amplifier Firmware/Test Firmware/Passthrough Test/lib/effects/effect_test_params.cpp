
#include <effect_test_params.h>

//=========================== STATIC MEMBER VARIABLES =======================

const Effect_Icon_t Effect_Test_Param::icon = {
    0xF8, 0xFF, 0xFF, 0x01, 0x06, 0x00, 0x00, 0x03, 0x22, 0x20, 0x20, 0x02, 
    0xF9, 0xF8, 0xF8, 0x04, 0x99, 0x99, 0xCC, 0x04, 0x8D, 0x8D, 0x8D, 0x05, 
    0x8D, 0x89, 0x8D, 0x04, 0xD9, 0xF9, 0xD8, 0x04, 0x71, 0x70, 0x78, 0x04, 
    0x01, 0x00, 0x00, 0x04, 0x01, 0x07, 0x07, 0x04, 0x81, 0x8F, 0x0F, 0x04, 
    0xC1, 0xD8, 0x18, 0x04, 0xC1, 0xD8, 0x18, 0x04, 0x81, 0x8C, 0x09, 0x04, 
    0x81, 0x8F, 0x0F, 0x04, 0x01, 0x02, 0x04, 0x04, 0x01, 0x00, 0x00, 0x04, 
    0x0F, 0x00, 0x80, 0x07, 0x31, 0x00, 0x60, 0x04, 0xF1, 0x01, 0x7C, 0x04, 
    0xF1, 0x03, 0x6E, 0x04, 0x0F, 0x07, 0x87, 0x07, 0x01, 0x0E, 0x03, 0x04, 
    0x01, 0x8C, 0x01, 0x04, 0x01, 0xFC, 0x01, 0x04, 0x01, 0xF8, 0x00, 0x04, 
    0x01, 0x20, 0x00, 0x04, 0x01, 0x00, 0x00, 0x04, 0x01, 0x00, 0x00, 0x04, 
    0x01, 0x00, 0x00, 0x04, 0x01, 0x00, 0x00, 0x04, 0x01, 0x00, 0x00, 0x04, 
    0x01, 0x00, 0x00, 0x04, 0x01, 0x00, 0x00, 0x04, 0x01, 0x00, 0x00, 0x04, 
    0x01, 0x00, 0x00, 0x04, 0x01, 0x00, 0x00, 0x04, 0x03, 0x00, 0x00, 0x02, 
    0x06, 0x00, 0x00, 0x03, 0xFC, 0xFF, 0xFF, 0x00
};

//=========================== OVERRIDDEN PUBLIC FUNCTIONS =========================

//save the name and effect theme color during initialization
Effect_Test_Param::Effect_Test_Param(RGB_LED::COLOR _theme_color, std::string _name):
    name(_name),
    theme_color(_theme_color),
    lin_param("Lin", -10, 10, 0.5, 1),
    log_param("Log", 10, 1000, 100, 100),
    sel_param("Sel", choices, choices[1]),
    effect_edit(to_return_page, leds, encs) //initialize our edit page implementation
{
    //set the header text and theme color for the effects edit menu
    effect_edit.set_display_text("Edit Dummy Params");
    effect_edit.set_LED_color(theme_color);

    //set the parameters to edit/render in the edit menu
    effect_edit.set_render_parmeter(&lin_param, 1);
    effect_edit.set_render_parmeter(&log_param, 2);
    effect_edit.set_render_parmeter(&sel_param, 3);
}

//copy constructor invokes the parameterized constructor above
Effect_Test_Param::Effect_Test_Param(const Effect_Test_Param& other):
    Effect_Test_Param(other.theme_color, other.name)
{}

void Effect_Test_Param::audio_update(const Audio_Block_t& block_in, Audio_Block_t& block_out) {
    //just copy the input block to the output
    std::copy(block_in.begin(), block_in.end(), block_out.begin());

    //and synchronize all of our params for rendering
    lin_param.synchronize();
    log_param.synchronize();
    sel_param.synchronize();
}

//function we call to actually instantiate a new effect on the heap
//return a unique pointer to automatically manage this heap memory
std::unique_ptr<Effect_Interface> Effect_Test_Param::clone() {
    return std::make_unique<Effect_Test_Param>(*new Effect_Test_Param(*this));
}

std::string Effect_Test_Param::get_name() { return name; }
Effect_Icon_t Effect_Test_Param::get_icon() { return icon; }
RGB_LED::COLOR Effect_Test_Param::get_theme_color() { return theme_color; }
Effect_Parameter* Effect_Test_Param::get_quick_edit_param() { return effect_edit.get_quick_edit_param(); }

//=========================== OVERRIDDEN PRIVATE FUNCTIONS =========================

//override our entry function --> call our default implementation
//configuration of render resources
void Effect_Test_Param::impl_on_entry() {
    //run the entry function in our edit menu implementation
    effect_edit.configure_render_resources();
}

//override our exit function --> call our default implementation
//release all the render resources
void Effect_Test_Param::impl_on_exit() {
    //run the exit function in our edit menu implementation 
    effect_edit.release_render_resources();
}


void Effect_Test_Param::draw() {
    //call the render function of our edit menu implemenation
    //pass it the global graphics handle
    effect_edit.render(graphics_handle);
}

