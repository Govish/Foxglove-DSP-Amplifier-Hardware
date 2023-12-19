#include <Arduino.h>

#include "audio_out_mqs.hpp"
#include "config.hpp"
#include <array>

volatile bool output_state = false;

void toggle(void* context) {
  if(output_state) {
    digitalWrite(19, HIGH);
    digitalWrite(15, HIGH);
  }
  else {
    digitalWrite(19, LOW);
    digitalWrite(15, LOW);
  }
    
  output_state = !output_state;
}

void sinewave(void* context) {
  std::array<int16_t, App_Constants::PROCESSING_BLOCK_SIZE> sine_buffer;
  static constexpr float FREQUENCY = 120;
  static constexpr float ACCUM_TO_RAD = TWO_PI/65536.0f;
  static constexpr uint16_t PHASE_INCREMENT = (uint16_t)(FREQUENCY/App_Constants::AUDIO_SAMPLE_RATE_HZ * 65536);
  static uint16_t phase_accumulator = 0; //rely on rollover to perform phase-wrapping

  for(size_t i = 0; i < App_Constants::PROCESSING_BLOCK_SIZE; i++) {
    phase_accumulator += PHASE_INCREMENT;
    sine_buffer[i] = (int16_t) (sin(((float)phase_accumulator) * ACCUM_TO_RAD) * 25000.0f);
  }

  Audio_Out_MQS::update(sine_buffer);
}

void setup() {
  //toggle this pin for DMA testing
  pinMode(15, OUTPUT);
  pinMode(19, OUTPUT);
  pinMode(14, OUTPUT);

  // put your setup code here, to run once:
  Audio_Out_MQS::init();

  //call the toggle function when we hit the DMA request interrupts
  Audio_Out_MQS::attach_interrupt(Context_Callback_Function<void>(nullptr, sinewave), App_Constants::AUDIO_BLOCK_PROCESS_PRIO);

  //start the DMA process
  Audio_Out_MQS::start();

}

void loop() {
  digitalWrite(14, LOW);
  delay(1000);
  digitalWrite(14, HIGH);
  delay(1000);
}