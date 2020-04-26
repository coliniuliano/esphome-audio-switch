#include "esphome.h"

#include <functional>

#define PIN_RESET   16
#define PIN_DATA    13
#define PIN_STROBE  12

#define PIN_AX0     32
#define PIN_AX1     33  
#define PIN_AX2     26
#define PIN_AY0     25  
#define PIN_AY1     14
#define PIN_AY2     27

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
    ESP_LOGD("CustomAudioMatrix", "Setup");

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
  }

  // Callback when a switch is toggled from Home Assistant
  void switch_toggled(uint8_t output, uint8_t input, bool state) {
    ESP_LOGD("custom_audio_matrix", "Switch toggled (%d, %d) %s", output, input, state ? "on" : "off");

    // Clear all other inputs from the output
    for (int x = 0; x < 8; x++)
      if (x != input)
        this->mt8808_send_data(x, output, false);

    // Set output to requested state
    this->mt8808_send_data(input, output, state);

    // Report states for all inputs for this output
    // NOTE: Only using a 3x3 matrix of switches at the moment
    if (output < 3) {
      for (int x = 0; x < 3; x++) {
        Switch *sw = this->switches.at(output * 3 + x);
        sw->publish_state(x == input ? state : false);
      }
    }
  }

private:
  CustomAudioMatrix() : Component() {
    using namespace std::placeholders;
    for (int output = 0; output < 3; output++) {
      for (int input = 0; input < 3; input++) {
        auto write_state_fn = std::bind(&CustomAudioMatrix::switch_toggled, this, output, input, _1);
        this->switches.push_back(new AudioMatrixSwitch(write_state_fn));
      }
    }
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