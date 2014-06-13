#include "TouchPin.h"

// How long the simulated strings vibrate
#define DEFAULT_STRING_VIBRATION_DURATION 2000L

class HarpString: 
public TouchPin {
  /**
   * A HarpString extends the TouchPin object and adds the concept of a "pluck"
   * operation. A pluck happens when the string is touched and then is released.
   */
#define STATE_IDLE 1
#define STATE_ARMED 2
#define STATE_SOUNDING 3

  boolean _state = STATE_IDLE;

  unsigned long _on_time = 0L;
  byte _midi_note = 0;
  unsigned long _duration = DEFAULT_STRING_VIBRATION_DURATION;  // How long the simulated string vibrates

public:
  HarpString(int pin, unsigned long sample_period) : 
  TouchPin(pin, sample_period) {
  };
  HarpString(int pin, unsigned long sample_period, unsigned int select_line) : 
  TouchPin(pin, sample_period, select_line) {
  };
  void update();
  void set_midi_note(byte note);
  void set_duration(unsigned long duration);
  virtual void note_on();
  virtual void note_off();
};
