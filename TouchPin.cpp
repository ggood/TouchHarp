#include "TouchPin.h"

TouchPin::TouchPin(int pin, unsigned long sample_period) {
  _pin = pin;
  _sample_period = sample_period;
  _last_sample_time = 0UL;
  _index = 0;
  _sum = 0UL;
  _select_line = -1;
  _touch_threshold = DEFAULT_TOUCH_THRESHOLD;
}


TouchPin::TouchPin(int pin, unsigned long sample_period, unsigned int select_line) {
  _pin = pin;
  _sample_period = sample_period;
  _last_sample_time = 0UL;
  _index= 0;
  _sum = 0UL;
  _select_line = select_line;
  _touch_threshold = DEFAULT_TOUCH_THRESHOLD;
}


void TouchPin::update() {
  if (millis() > _last_sample_time + _sample_period) {
    if (_select_line >= 0) {
      // Select the appropriate input on the multiplexer
      digitalWrite(SEL0, _select_line & 0x01);
      digitalWrite(SEL1, (_select_line >> 1)  & 0x01);
      digitalWrite(SEL2, (_select_line >> 2)  & 0x01);
      digitalWrite(SEL3, (_select_line >> 3)  & 0x01);
    }
    // Remember the time we read this value
    _last_sample_time = millis();
    // Read the value
    unsigned int val = touchRead(_pin);
    // Update the running average
    _sum -= _samples[_index];
    _samples[_index] = val;
    _sum += val;
    _index = (_index + 1 ) % SAMPLE_BUFFER_SIZE;
  }
}


unsigned int TouchPin::value() {
  // Return the running average
  return _sum / SAMPLE_BUFFER_SIZE;
}


boolean TouchPin::touching() {
  // If the current value is above the touch threshold, return true
  return value() > _touch_threshold;
}


void TouchPin::set_touch_threshold(unsigned int touch_threshold) {
  // Set the threshold value - if the value we read is larger than
  // touch_threshold, we report a touch event.
  _touch_threshold = touch_threshold;
}
