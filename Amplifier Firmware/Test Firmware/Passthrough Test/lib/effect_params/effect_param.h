#pragma once

/*
 * A parameter of an audio effect
 * A quick class that manages everything related to an audio effect parameter, namely:
 *  - configuring an encoder to adjust said parameter
 *  - lighting an LED when the effect is being adjusted
 *  - synchronizing the effect during an audio update
 *  - drawing the paramter on the OLED when it's selected
 *  - drawing the parameter on the OLED when it's deselected
 * Functions of this class can be overridden to provide different draw methods and such
 * 
 * By Ishaan Gov December 2023
 */

#include <string>
#include <Arduino.h>
#include <U8g2lib.h>

#include <ui_page.h> //for some render-related function
#include <encoder.h>

//all methods will basically be implementation defined
class Effect_Parameter {
public:
    //default constructor just stores the label of the parameter
    Effect_Parameter(const std::string _label): label(_label) {}

    //forward destructor to derived classes
    virtual ~Effect_Parameter() {}

    //hook up an encoder to tweak said parameter
    //should be able to use this function for both `edit` and `quick_edit`
    //detaching encoder clears the pointer
    virtual void attach_configure_enc(Rotary_Encoder& _enc) { enc = &_enc; }
    virtual void detach_enc() { enc = nullptr; }
    
    //render the particular parameter in the normal edit context
    virtual void draw(uint32_t x_offset, uint32_t y_offset, U8G2& graphics_handle) {}

    //render the particular parameter in the quick edit context
    virtual void draw_quick_edit(uint32_t x_offset, uint32_t y_offset, U8G2& graphics_handle) {}

    //synchronize the parameter readback variable to the knob position
    //this basically mitigates the situation where the parameter is used in one part of the audio update
    //then the parameter is changed by the user before the audio update can complete
    // ^ the above situation should theoretically never happen due to the way priorities and threading likely works
    //   but doing this just in case
    virtual void synchronize() {}

    //provide methods to get the bounding box width and height of the parameter
    inline uint32_t get_edit_width() { return PARAM_EDIT_RENDER_WIDTH; }
    inline uint32_t get_edit_height() { return PARAM_EDIT_RENDER_HEIGHT; }
    inline uint32_t get_quick_edit_width() { return PARAM_QE_RENDER_WIDTH; }
    inline uint32_t get_quick_edit_height() { return PARAM_QE_RENDER_HEIGHT; }
    //along with the parameter label
    inline std::string get_label() { return label; }

protected:
    //point to an encoder that will be used to modify our parameter
    Rotary_Encoder* enc;

    //and store the label of the parameter
    const std::string label;

    //dimensions of the parameter in px when rendered on the screen in normal edit context
    const uint32_t PARAM_EDIT_RENDER_WIDTH = 24;
    const uint32_t PARAM_EDIT_RENDER_HEIGHT = 49;

    //dimensions of the parameter in px when rendered on the screen in quick edit context 
    const uint32_t PARAM_QE_RENDER_WIDTH = 128;
    const uint32_t PARAM_QE_RENDER_HEIGHT = 64;
};