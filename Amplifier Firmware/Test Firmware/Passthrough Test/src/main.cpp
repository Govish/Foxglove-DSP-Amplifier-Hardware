#include <array>
#include <Arduino.h>
#include <U8g2lib.h>

//helper function includes
#include <all_effects.h>
#include <ui_system.h>

//Utility-type things includes
#include <config.h>
#include <scheduler.h>

//hardware includes
#include <audio_out_mqs.h>
#include <audio_in_adc.h>
#include <rgb.h>
#include <encoder.h>

//instantiate our RGB LEDs
RGB_LED led_1(Pindefs::LED_CHAN1_R, Pindefs::LED_CHAN1_G, Pindefs::LED_CHAN1_B, Pindefs::RGB_ACTIVE_HIGH);
RGB_LED led_2(Pindefs::LED_CHAN2_R, Pindefs::LED_CHAN2_G, Pindefs::LED_CHAN2_B, Pindefs::RGB_ACTIVE_HIGH);
RGB_LED led_3(Pindefs::LED_CHAN3_R, Pindefs::LED_CHAN3_G, Pindefs::LED_CHAN3_B, Pindefs::RGB_ACTIVE_HIGH);
RGB_LED led_4(Pindefs::LED_CHAN4_R, Pindefs::LED_CHAN4_G, Pindefs::LED_CHAN4_B, Pindefs::RGB_ACTIVE_HIGH);
RGB_LED led_main(Pindefs::LED_MAIN_R, Pindefs::LED_MAIN_G, Pindefs::LED_MAIN_B, Pindefs::RGB_ACTIVE_HIGH);
std::array<RGB_LED*, 5> RGB_LEDs = {&led_1, &led_2, &led_3, &led_4, &led_main}; //aggregate all of them

//instantiate our encoders
Rotary_Encoder enc_1(Pindefs::ENC_CHAN1_A, Pindefs::ENC_CHAN1_B, Pindefs::ENC_CHAN1_SW, Rotary_Encoder::X1_FWD);
Rotary_Encoder enc_2(Pindefs::ENC_CHAN2_A, Pindefs::ENC_CHAN2_B, Pindefs::ENC_CHAN2_SW, Rotary_Encoder::X1_FWD);
Rotary_Encoder enc_3(Pindefs::ENC_CHAN3_A, Pindefs::ENC_CHAN3_B, Pindefs::ENC_CHAN3_SW, Rotary_Encoder::X1_FWD);
Rotary_Encoder enc_4(Pindefs::ENC_CHAN4_A, Pindefs::ENC_CHAN4_B, Pindefs::ENC_CHAN4_SW, Rotary_Encoder::X1_FWD);
Rotary_Encoder enc_main(Pindefs::ENC_MAIN_A, Pindefs::ENC_MAIN_B, Pindefs::ENC_MAIN_SW, Rotary_Encoder::X1_FWD);
std::array<Rotary_Encoder*, 5> encoders = {&enc_1, &enc_2, &enc_3, &enc_4, &enc_main}; //aggregate all of them

//create the interface to the OLED using the U8G2 library
//get the specific display type and rotation from the config 
//TODO: FIX!!! Another PlatformIO include path issue
//App_Constants::U8G2_LCD_TYPE ui_display(App_Constants::SCREEN_ROTATION);
U8G2_SH1106_128X64_NONAME_F_HW_I2C ui_display(U8G2_R0);

//initialize the static variables in the UI_Page parent class
//includes default fonts, LEDs and Encoders
U8G2& UI_Page::graphics_handle = ui_display;
const uint8_t* UI_Page::DEFAULT_FONT = u8g2_font_spleen6x12_me;
const uint8_t* UI_Page::SMALL_FONT = u8g2_font_04b_03_tr;
std::array<RGB_LED*, App_Constants::NUM_RGB_LEDs>& UI_Page::leds = RGB_LEDs;
std::array<Rotary_Encoder*, App_Constants::NUM_ENCODERS>& UI_Page::encs = encoders;

//this corresponds to our main audio system update!
void audio_system_update() {
	//statically allocate some storate to keep all of our sample buffers
	static std::array<Audio_Block_t, App_Constants::NUM_EFFECTS + 1> effect_buffers;

	//read the data in from the ADC
	Audio_In_ADC::get_samples(effect_buffers[0]);

	//run the audio samples through the effect chain
	//input from the array at the current index
	//output to the array at the next index
	for(size_t i = 0; i < App_Constants::NUM_EFFECTS; i++)
		Effects_Manager::get_active_effect(i)->audio_update(
			effect_buffers[i],	//read from current index
			effect_buffers[i+1]	//write to next index
		);

	//write the data out with the processed audio data from the last effect
	Audio_Out_MQS::update(effect_buffers[App_Constants::NUM_EFFECTS]);
}

void setup() {
	//initialize our audio effects
	Effects_Manager::init();

	//initialize our display and UI system
	ui_display.setI2CAddress(App_Constants::DISPLAY_I2C_ADDRESS);
	ui_display.begin();
	UI_System::make_ui();

	//initialize our RGB LEDs
	for(RGB_LED* led : RGB_LEDs) led->init();

	//initialize our encoders
	for(Rotary_Encoder* enc : encoders) enc->init();

	//initialize the input/output hardware
	Audio_Out_MQS::init();
	Audio_In_ADC::init();

	//attach the update hook to the output function --> this corresponds to the main audio system update!
	//should run at the highest priority to provide the most real time operation
	Audio_Out_MQS::attach_interrupt(audio_system_update, App_Constants::AUDIO_BLOCK_PROCESS_PRIO);

	//start the DMA process for input and output sampling
	//I don't think the order of this should *really* matter
	//but there's a possibility of some edge-cases about which half of the buffer the ADC update is caught in
	//and I think performance is more guaranteed if we start the ADC first
	Audio_In_ADC::start();
	Audio_Out_MQS::start();

	//start our UI system
	UI_System::start();
}

void loop() {
	//all we need to do in the loop is run our scheduler and encoder callbacks
	//everything else is managed by the UI system
	//and audio updates run in interrupt context; so don't need to take place here
	Rotary_Encoder::update_all();
	Scheduler::update();
}