
#include <effect_test_passthrough.h>

//=========================== STATIC MEMBER VARIABLES =======================

const Effect_Icon_t Effect_Test_Passthrough::icon = {
    0xFC, 0xFF, 0xFF, 0x01, 0x06, 0x00, 0x00, 0x03, 0x03, 0x00, 0x00, 0x06, 
    0x01, 0x00, 0x00, 0x04, 0x01, 0x00, 0x00, 0x04, 0x01, 0x00, 0x00, 0x04, 
    0x01, 0x00, 0x00, 0x04, 0x01, 0x00, 0x00, 0x04, 0x01, 0x00, 0x00, 0x04, 
    0x01, 0xF8, 0x00, 0x04, 0x01, 0xFC, 0x01, 0x04, 0x01, 0x8C, 0x01, 0x04, 
    0x01, 0x8C, 0x01, 0x04, 0x01, 0x0C, 0x03, 0x04, 0x01, 0x0C, 0x03, 0x04, 
    0x01, 0x18, 0x03, 0x04, 0x01, 0x18, 0x07, 0x04, 0x01, 0x38, 0x06, 0x04, 
    0x0F, 0x30, 0x86, 0x07, 0x11, 0x30, 0x5C, 0x04, 0xF1, 0x61, 0x78, 0x04, 
    0xF1, 0x61, 0x60, 0x04, 0x0F, 0x63, 0x80, 0x07, 0x01, 0xC3, 0x00, 0x04, 
    0x01, 0xC3, 0x00, 0x04, 0x01, 0xC3, 0x00, 0x04, 0x01, 0xC6, 0x00, 0x04, 
    0x01, 0xC6, 0x00, 0x04, 0x01, 0xFE, 0x00, 0x04, 0x01, 0x7C, 0x00, 0x04, 
    0x01, 0x00, 0x00, 0x04, 0x01, 0x00, 0x00, 0x04, 0x01, 0x00, 0x00, 0x04, 
    0x01, 0x00, 0x00, 0x04, 0x01, 0x00, 0x00, 0x04, 0x01, 0x00, 0x00, 0x04, 
    0x01, 0x00, 0x00, 0x04, 0x01, 0x00, 0x00, 0x04, 0x03, 0x00, 0x00, 0x02, 
    0x06, 0x00, 0x00, 0x03, 0xFC, 0xFF, 0xFF, 0x00
};

//=========================== OVERRIDDEN PUBLIC FUNCTIONS =========================

//save the name and effect theme color during initialization
Effect_Test_Passthrough::Effect_Test_Passthrough(RGB_LED::COLOR _theme_color, std::string _name):
    name(_name),
    theme_color(_theme_color)
{}

void Effect_Test_Passthrough::audio_update(const Audio_Block_t& block_in, Audio_Block_t& block_out) {
    //just copy the input block to the output
    std::copy(block_in.begin(), block_in.end(), block_out.begin());
}

//function we call to actually instantiate a new effect on the heap
//return a unique pointer 
std::unique_ptr<Effect_Interface> Effect_Test_Passthrough::clone() {
    return std::make_unique<Effect_Test_Passthrough>(*new Effect_Test_Passthrough(*this));
}

std::string Effect_Test_Passthrough::get_name() { return name; }
Effect_Icon_t Effect_Test_Passthrough::get_icon() { return icon; }
RGB_LED::COLOR Effect_Test_Passthrough::get_theme_color() { return theme_color; }

//=========================== OVERRIDDEN PRIVATE FUNCTIONS =========================

//override the entry function, schedule a transition after one second
void Effect_Test_Passthrough::impl_on_entry() {
    //schedule a transition out of the current page
    //since we don't have any parameters to attach
    done_editing_sched.schedule_oneshot_ms(to_return_page, 1000);
}

void Effect_Test_Passthrough::draw() {
    //display a message on the screen
    static const std::array<std::string, 2> no_param_msg = {
        "This Effect Has No",
        "Editable Parameters"
    };

    //compute the coordinates of the text such that it's center aligned
    static std::array<u8g2_uint_t, no_param_msg.size()> x_coords;
    static std::array<u8g2_uint_t, no_param_msg.size()> y_coords;
    for(size_t i = 0; i < no_param_msg.size(); i++) 
        //center the text
        x_coords[i] = ( graphics_handle.getWidth() - 
                        graphics_handle.getStrWidth(no_param_msg[i].c_str())) >> 1;
    y_coords[0] = 20;
    y_coords[1] = y_coords[0] + graphics_handle.getMaxCharHeight() + 2;

    //render the text on the screen
    graphics_handle.clearBuffer();
    for(size_t i = 0; i < no_param_msg.size(); i++) 
        graphics_handle.drawStr(x_coords[i], y_coords[i], no_param_msg[i].c_str());
    graphics_handle.sendBuffer();
}

