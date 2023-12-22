#include <Arduino.h>

#include "audio_out_mqs.hpp"
#include "audio_in_adc.hpp"

#include "config.hpp"
#include <array>

volatile bool output_state = false;

// void toggle(void* context) {
//   if(output_state) {
//     digitalWrite(19, HIGH);
//     digitalWrite(15, HIGH);
//   }
//   else {
//     digitalWrite(19, LOW);
//     digitalWrite(15, LOW);
//   }
    
//   output_state = !output_state;
// }

void sinewave(void* context) {
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

void passthrough(void* context) {
  std::array<int16_t, App_Constants::PROCESSING_BLOCK_SIZE> sample_buffer;
  Audio_In_ADC::get_samples(sample_buffer);
  Audio_Out_MQS::update(sample_buffer);
}

void setup() {
  //toggle this pin for DMA testing
  pinMode(15, OUTPUT);
  pinMode(19, OUTPUT);
  pinMode(14, OUTPUT);

  digitalWrite(15, HIGH);
  digitalWrite(14, HIGH);
  digitalWrite(19, HIGH);

  //TODO: move this to Audio_In_ADC!
  //REALLY IMPORTANT!
  pinMode(A2, INPUT);

  //initialize the input/output hardware
  Audio_Out_MQS::init();
  Audio_In_ADC::init();

  //attach the update hook to the output function --> this corresponds to the main audio system update!
  //should run at the highest priority to provide the most real time operation
  //Audio_Out_MQS::attach_interrupt(Context_Callback_Function<void>(nullptr, sinewave), App_Constants::AUDIO_BLOCK_PROCESS_PRIO);
  Audio_Out_MQS::attach_interrupt(Context_Callback_Function<void>(nullptr, passthrough), App_Constants::AUDIO_BLOCK_PROCESS_PRIO);

  //start the DMA process for input and output sampling
  //I don't think the order of this should *really* matter
  //but there's a possibility of some edge-cases about which half of the buffer the ADC update is caught in
  //and I think performance is more guaranteed if we start the ADC first
  Audio_In_ADC::start();
  Audio_Out_MQS::start();
}

void loop() {
  digitalWrite(14, LOW);
  delay(1000);
  digitalWrite(14, HIGH);
  delay(1000);
}