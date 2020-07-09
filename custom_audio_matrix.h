#include "esphome.h"

#include <functional>

#define PIN_RESET   19
#define PIN_DATA    18
#define PIN_STROBE  17

#define PIN_AX0     26
#define PIN_AX1     27
#define PIN_AX2     14
#define PIN_AY0     12
#define PIN_AY1     13
#define PIN_AY2     16

class AudioMatrixSwitch : public Switch {
protected:
  std::function<void(bool)> write_state_fn;
public:
  AudioMatrixSwitch(std::function<void(bool)> write_state_fn) {
    this->write_state_fn = write_state_fn;
  }

  void write_state(bool state) override {
    write_state_fn(state);
  }
};

// Matrix is ordered by output:
// OUT0 : IN0, IN1, ..., IN7
// OUT1 : IN0, IN1, ..., IN7
// ...
// OUT7 : IN0, IN1, ...

class CustomAudioMatrix : public Component {
public:
  std::vector<Switch*> switches;

  static CustomAudioMatrix* instance() {
    static CustomAudioMatrix* INST = new CustomAudioMatrix();
    return INST;
  }

  void setup() override {
    pinMode(PIN_RESET, OUTPUT);
    digitalWrite(PIN_RESET, LOW);

    pinMode(PIN_DATA, OUTPUT);
    pinMode(PIN_STROBE, OUTPUT);
    digitalWrite(PIN_STROBE, HIGH);
    
    pinMode(PIN_AX0, OUTPUT);
    pinMode(PIN_AX1, OUTPUT);
    pinMode(PIN_AX2, OUTPUT);
    pinMode(PIN_AY0, OUTPUT);
    pinMode(PIN_AY1, OUTPUT);
    pinMode(PIN_AY2, OUTPUT);

    digitalWrite(PIN_RESET, HIGH);
    ESP_LOGD("CustomAudioMatrix", "Setup complete");
  }

  // Callback when a switch is toggled from Home Assistant
  void switch_toggled(uint8_t output, uint8_t input, bool state) {
    ESP_LOGD("custom_audio_matrix", "Switch toggled (%d, %d) %s", output, input, state ? "on" : "off");

    // Clear all other inputs from the output
    // NOTE: Inputs are on Y, outputs are on X
    for (int x = 0; x < 8; x++)
      if (x != input)
        this->mt8808_send_data(output, x, false);

    // Set output to requested state
    this->mt8808_send_data(output, input, state);

    // Report states for all inputs for this output
    // NOTE: Hacky - switch at (output - 1) will be for input 4, while switch at (output) is for input 5
    Switch *sw = this->switches.at(output);
    sw->publish_state(input == 5 ? state : false);
    sw = this->switches.at(output - 1);
    sw->publish_state(input == 4 ? state : false);
  }

private:
  CustomAudioMatrix() : Component() {
    this->create_switch(1, 4);
    this->create_switch(1, 5);
    this->create_switch(3, 4);
    this->create_switch(3, 5);
    this->create_switch(5, 4);
    this->create_switch(5, 5);
    this->create_switch(7, 4);
    this->create_switch(7, 5);
  }

  void create_switch(uint8_t output, uint8_t input) {
    using namespace std::placeholders;
    auto write_state_fn = std::bind(&CustomAudioMatrix::switch_toggled, this, output, input, _1);
    this->switches.push_back(new AudioMatrixSwitch(write_state_fn));
  }

  void mt8808_send_data(uint8_t addr_x, uint8_t addr_y, bool switch_en) {
    ESP_LOGD("custom_audio_matrix", "MT880x setting %d_%d %s", addr_x, addr_y, switch_en ? "on" : "off");
    digitalWrite(PIN_AX0, addr_x & 0x01);
    digitalWrite(PIN_AX1, addr_x & 0x02);
    digitalWrite(PIN_AX2, addr_x & 0x04);
    digitalWrite(PIN_AY0, addr_y & 0x01);
    digitalWrite(PIN_AY1, addr_y & 0x02);
    digitalWrite(PIN_AY2, addr_y & 0x04);

    digitalWrite(PIN_DATA, switch_en);

    digitalWrite(PIN_STROBE, LOW);
    delay(10);
    digitalWrite(PIN_STROBE, HIGH);
  }
};