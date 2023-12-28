#include <Arduino.h>
#include <array>

#include "config.hpp"
#include "scheduler.hpp"
#include "audio_out_mqs.hpp"
#include "audio_in_adc.hpp"
#include "rgb.hpp"
#include "encoder.hpp"

//instantiate our RGB LEDs
RGB_LED led_1(Pindefs::LED_CHAN1_R, Pindefs::LED_CHAN1_G, Pindefs::LED_CHAN1_B, Pindefs::RGB_ACTIVE_HIGH);
RGB_LED led_2(Pindefs::LED_CHAN2_R, Pindefs::LED_CHAN2_G, Pindefs::LED_CHAN2_B, Pindefs::RGB_ACTIVE_HIGH);
RGB_LED led_3(Pindefs::LED_CHAN3_R, Pindefs::LED_CHAN3_G, Pindefs::LED_CHAN3_B, Pindefs::RGB_ACTIVE_HIGH);
RGB_LED led_4(Pindefs::LED_CHAN4_R, Pindefs::LED_CHAN4_G, Pindefs::LED_CHAN4_B, Pindefs::RGB_ACTIVE_HIGH);
RGB_LED led_main(Pindefs::LED_MAIN_R, Pindefs::LED_MAIN_G, Pindefs::LED_MAIN_B, Pindefs::RGB_ACTIVE_HIGH);
constexpr std::array<RGB_LED*, 5> RGB_LEDs = {&led_1, &led_2, &led_3, &led_4, &led_main}; //aggregate all of them

//instantiate our encoders
Rotary_Encoder enc_1(Pindefs::ENC_CHAN1_A, Pindefs::ENC_CHAN1_B, Pindefs::ENC_CHAN1_SW, Rotary_Encoder::X1_FWD);
Rotary_Encoder enc_2(Pindefs::ENC_CHAN2_A, Pindefs::ENC_CHAN2_B, Pindefs::ENC_CHAN2_SW, Rotary_Encoder::X1_FWD);
Rotary_Encoder enc_3(Pindefs::ENC_CHAN3_A, Pindefs::ENC_CHAN3_B, Pindefs::ENC_CHAN3_SW, Rotary_Encoder::X1_FWD);
Rotary_Encoder enc_4(Pindefs::ENC_CHAN4_A, Pindefs::ENC_CHAN4_B, Pindefs::ENC_CHAN4_SW, Rotary_Encoder::X1_FWD);
Rotary_Encoder enc_main(Pindefs::ENC_MAIN_A, Pindefs::ENC_MAIN_B, Pindefs::ENC_MAIN_SW, Rotary_Encoder::X1_FWD);
constexpr std::array<Rotary_Encoder*, 5> encs = {&enc_1, &enc_2, &enc_3, &enc_4, &enc_main}; //aggregate all of them

//create a couple schedulers so we can repeat some tasks
Scheduler flash_led1;
Scheduler flash_led2;
Scheduler flash_led3;
Scheduler flash_led4;
Scheduler flash_ledmain;
constexpr std::array<Scheduler*, 5> scheds = {&flash_led1, &flash_led2, &flash_led3, &flash_led4, &flash_ledmain};

void sinewave() {
  std::array<int16_t, App_Constants::PROCESSING_BLOCK_SIZE> sine_buffer;
  static constexpr float FREQUENCY = 187.5;
  static constexpr float ACCUM_TO_RAD = TWO_PI/65536.0f;
  static constexpr uint16_t PHASE_INCREMENT = (uint16_t)(FREQUENCY/(float)App_Constants::AUDIO_SAMPLE_RATE_HZ * 65536.0);
  static uint16_t phase_accumulator = 0; //rely on rollover to perform phase-wrapping

  for(size_t i = 0; i < App_Constants::PROCESSING_BLOCK_SIZE; i++) {
    phase_accumulator += PHASE_INCREMENT;
    sine_buffer[i] = (int16_t) (sin(((float)phase_accumulator) * ACCUM_TO_RAD) * 25000.0f);
  }

  Audio_Out_MQS::update(sine_buffer);
}

void passthrough() {
  std::array<int16_t, App_Constants::PROCESSING_BLOCK_SIZE> sample_buffer;
  Audio_In_ADC::get_samples(sample_buffer);
  Audio_Out_MQS::update(sample_buffer);
}

void clear_led(void* context) {
  uint32_t which_enc = reinterpret_cast<uint32_t>(context);
  RGB_LEDs[which_enc]->set_color(RGB_LED::OFF); //clear the corresponding LED
}

void print_encoders(void* context) {
  uint32_t which_enc = reinterpret_cast<uint32_t>(context);
  RGB_LEDs[which_enc]->set_color(RGB_LED::GREEN); //flash the corresponding LED green
  scheds[which_enc]->schedule_oneshot_ms(Context_Callback_Function<void>(context, clear_led), 250); //turn it off after 250ms

  for(Rotary_Encoder* enc : encs) {
    Serial.print(enc->get_counts());
    Serial.print('\t');
  }
  Serial.println();
}

void on_click(void* context) {
  uint32_t which_enc = reinterpret_cast<uint32_t>(context);
  RGB_LEDs[which_enc]->set_color(RGB_LED::RED); //flash the corresponding LED RED
  scheds[which_enc]->schedule_oneshot_ms(Context_Callback_Function<void>(context, clear_led), 250); //turn it off after 250ms

  Rotary_Encoder* enc = encs[which_enc];
  enc->set_counts(0);
}

void setup() {
  //debugging
  Serial.begin(115200);


  //initialize our RGB LEDs
  for(RGB_LED* led : RGB_LEDs) led->init();

  //initialize our encoders
  for(uint32_t i = 0; i < encs.size(); i++) {
    Rotary_Encoder* enc = encs[i];
    enc->init();
    enc->set_max_counts(100);
    enc->attach_on_change(Context_Callback_Function<void>(reinterpret_cast<void*>(i), print_encoders));
    enc->attach_on_press(Context_Callback_Function<void>(reinterpret_cast<void*>(i), on_click));
  }
  

  //initialize the input/output hardware
  Audio_Out_MQS::init();
  Audio_In_ADC::init();

  //attach the update hook to the output function --> this corresponds to the main audio system update!
  //should run at the highest priority to provide the most real time operation
  //Audio_Out_MQS::attach_interrupt(Context_Callback_Function<void>(sinewave), App_Constants::AUDIO_BLOCK_PROCESS_PRIO);
  Audio_Out_MQS::attach_interrupt(Context_Callback_Function<void>(passthrough), App_Constants::AUDIO_BLOCK_PROCESS_PRIO);

  //start the DMA process for input and output sampling
  //I don't think the order of this should *really* matter
  //but there's a possibility of some edge-cases about which half of the buffer the ADC update is caught in
  //and I think performance is more guaranteed if we start the ADC first
  Audio_In_ADC::start();
  Audio_Out_MQS::start();
}

void loop() {
  Rotary_Encoder::update_all(); //run encoder callbacks
  Scheduler::update(); //run all scheduled tasks
}