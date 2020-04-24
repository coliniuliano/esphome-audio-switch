#include "esphome.h"

class CustomAudioSwitch : public Component, public Switch {
public:
  void setup() override {
    // This will be called by App.setup()
    pinMode(5, INPUT);
  }

  // This will be called every time the user requests a state change.
  void write_state(bool state) override {
    digitalWrite(5, state);

    // Acknowledge new state by publishing it
    publish_state(state);
  }
};