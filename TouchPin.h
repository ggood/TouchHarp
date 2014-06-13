#include "Arduino.h"

// How many samples to average
#define SAMPLE_BUFFER_SIZE 10
// Default reading from a touch sensor to consider a touch event
#define DEFAULT_TOUCH_THRESHOLD 5000
// Multiplexer select pins
#define SEL0 2
#define SEL1 3
#define SEL2 4
#define SEL3 5

class TouchPin {
  /**
   * This class handles input on a single Teensy touch input pin, and does
   * de-glitching by keeping a ring buffer of input values. The value
   * returned by the value() method is a running average of the last
   * SAMPLE_BUFFER_SIZE samples, sampled every _sample_period milliseconds.
   * The caller is reponsible for calling the update() method at least once
   * every _sample_period milliseconds.
   */
  int _pin;  // Touch input pin on Teensy
  unsigned long _sample_period;  // Sample input every _sample_period millisconds
  unsigned long _last_sample_time;
  unsigned int _samples[SAMPLE_BUFFER_SIZE];  // Ring buffer of touchRead values
  unsigned int _index;  // Current index into ring buffer
  unsigned long _sum;  // Current sum
  unsigned int _touch_threshold;  // Threshold for declaring a touched event
  
protected:
  int _select_line;  // If > 0, select this input pin on the multiplexer

public:
  TouchPin(int, unsigned long);
  TouchPin(int, unsigned long, unsigned int);
  virtual void update();
  unsigned int value();
  boolean touching();
  void set_touch_threshold(unsigned int touch_threshold);
};
