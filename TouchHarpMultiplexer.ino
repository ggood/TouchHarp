/**
 * 
 * The Touch Harp is an instrument played like a harp, but uses
 * touch sensitive input on a Teensy 3 microcontroller.
 * 
 * The Teensy3 has 9 pins that can do touch sensing input. Since
 * a traditional harp has many more strings, we need some way
 * to allow more than 9 "strings" to be sensed.
 * 
 * An analog multiplexer chip like to CD4067 (1 x 16) allows one
 * touch input pin on the Teensy to handle 16 separate touch
 * inputs if you periodically select each input and record
 * the current value. To select which one of the 16 inputs on
 * the 4067 is active, you need to write values to the four
 * select pins on the chip, e.g.
 * 
 * S3 S2 S1 S0
 * 0  0  0  0 - input 0 is selected
 * 0  0  0  1 - input 1 is selected
 * 0  0  1  0 - input 2 is selected
 * 
 * and so on, up to
 * 
 * 1  1  1  1 - input 15 is selected
 * 
 * This skectch assumes the following connections between the
 * Teensy 3.0 pins and the input select pins on the analog
 * multiplexer:
 * 
 * Teensy  4067
 * ======  ====
 * 2    10
 * 3    11
 * 4    14
 * 5    13
 * 
 * A datasheet for the 4067 is available online at:
 * https://www.sparkfun.com/datasheets/IC/CD74HC4067.pdf
 * 
 * You can think of the 4067 like an old school rotary
 * switch that connects an output pin to one of 16 input
 * pins. So connect the output pin of the 4067 to a
 * touch-input-capable pin (pin 23, aka A9 is used in this
 * sketch). Then connect your touch sensors to the inputs
 * on the 4067, labeled i0 through i15.
 * 
 * In this sketch, only the first 13 inputs on the 4067
 * are used, since that's how many "harp strings" my
 * instrument has, although I'm expanding it to use 3
 * multiplexers. Eventually, this will support 48 strings
 * total, but for now it's 3x13 = 39.
 * 
 * The value that the Teensy TouchRead() method returns
 * can vary a lot depending on how much stray capacitance
 * is present in the system, so the treshold above which
 * we declare an input to be touched can be set via a
 * potentiometer which is read on analog input A0.
 * 
 * Another potentiometer attached to analog input A1 controls
 * how log the MIDI note remains on after a simulated string
 * is plucked. For patches that already mimic a plucked
 * instrument, you can set this to a very short duration.
 * For patches like synth pads, setting the duration to
 * a few seconds can produce some interesting effects.
 * 
 */

// Touch input pins. There are 3 multiplexers, so
// there are 3 touch input pins
#define TOUCH_IN_0 A9
#define TOUCH_IN_1 A8
#define TOUCH_IN_2 A5

// Each multiplexer handles 13 input pins for now, so
// there are 13x3 = 39 total notes
#define NUM_STRINGS 39

// LED output pins
#define LED0 13

// Multiplexer select pins
#define SEL0 2
#define SEL1 3
#define SEL2 4
#define SEL3 5

// Sensitivity adjustment input pin
#define SENS_IN A0
// Duration adjustment input pin
#define DUR_IN A1
// Sample period - sample input no more often than this (in milliseconds)
#define SAMPLE_PERIOD 1
// How many samples to average
#define SAMPLE_BUFFER_SIZE 10
// Default reading from a touch sensor to consider a touch event
#define DEFAULT_TOUCH_THRESHOLD 5000
// How long the simulated strings vibrate
#define DEFAULT_STRING_VIBRATION_DURATION 2000L

// Imports and definitions related to LED feedback support

#include "FastLED.h"

// How many leds in your strip?
#define NUM_LEDS 39

// The pin used for communicating with the LED string
#define DATA_PIN 6

// Define the array of leds
CRGB leds[NUM_LEDS];


// Formatted print support, just for debug messages
#include <stdarg.h>
void p(char *fmt, ... ){
  char tmp[128]; // resulting string limited to 128 chars
  va_list args;
  va_start (args, fmt );
  vsnprintf(tmp, 128, fmt, args);
  va_end (args);
  Serial.print(tmp);
}


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
  // XXX(ggood) this should actually be a per-led offset

public:
  TouchPin(int, unsigned long);
  TouchPin(int, unsigned long, unsigned int);
  virtual void update();
  unsigned int value();
  boolean touching();
  void set_touch_threshold(unsigned int touch_threshold);
};

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


void HarpString::update() {
  TouchPin::update();  // Call superclass, where the pin is actually read
  switch (_state) {
  case STATE_IDLE:
    if (touching()) {
      _state = STATE_ARMED;
    }
    break;
  case STATE_ARMED:
    if (!touching()) {
      note_on();
      _state = STATE_SOUNDING;
      _on_time = millis();
    }
    break;
  case STATE_SOUNDING:
    if (touching()) {
      // Stop string "vibration"
      note_off();
      _state = STATE_ARMED;
    } 
    else if (millis() - _on_time > _duration) {
      note_off();
      _on_time = 0L;
      _state = STATE_IDLE;
    }
    break;
  default:
    Serial.println("OOOOPS");
  }
}


void HarpString::note_on() {
  usbMIDI.sendNoteOn(_midi_note, 100, 1);
}


void HarpString::note_off() {
  usbMIDI.sendNoteOff(_midi_note, 100, 1);
}


void HarpString::set_midi_note(byte midi_note) {
  _midi_note = midi_note;
}


void HarpString::set_duration(unsigned long duration) {
  // Set the duration, in milliseonds, that the
  // simulated string will vibrate
  _duration = duration;
}

class LEDHarpString: 
public HarpString {
  /**
   * The LEDHarpString extends the Harpstring object and adds the ability to
   * update an LED state based on the note on state.
   */
public:
  LEDHarpString(int pin, unsigned long sample_period) : 
  HarpString(pin, sample_period) {
  };
  LEDHarpString(int pin, unsigned long sample_period, unsigned int select_line) : 
  HarpString(pin, sample_period, select_line) {
  };
  void set_hue(byte);
  virtual void note_on();
  virtual void note_off();
  virtual void update();
private:
  unsigned long _note_on_time = 0UL;
  unsigned int _fade_time = 1000;
  byte _hue = 200;
};

void LEDHarpString::note_on() {
  HarpString::note_on();
  _note_on_time = millis();
};

void LEDHarpString::note_off() {
  HarpString::note_off();
  _note_on_time = 0UL;
};

void LEDHarpString::set_hue(byte hue) {
  _hue = hue;
}

void LEDHarpString::update() {
  HarpString::update();
  if (_note_on_time > 0UL) {
    unsigned long elapsed = constrain(millis() - _note_on_time, 0, _fade_time);
    byte value = map(elapsed, 0, _fade_time, 255, 0);
    leds[_select_line] = CHSV(_hue, 255, value);
  } else {
    leds[_select_line] = CHSV(_hue, 255, 0);
  }
}



// The current prototype has NUM_STRINGS "strings", all attached to
// analog input 9 on the Teensy
LEDHarpString strings[39] = {
  LEDHarpString(TOUCH_IN_0, SAMPLE_PERIOD, 0),
  LEDHarpString(TOUCH_IN_0, SAMPLE_PERIOD, 1),
  LEDHarpString(TOUCH_IN_0, SAMPLE_PERIOD, 2),
  LEDHarpString(TOUCH_IN_0, SAMPLE_PERIOD, 3),
  LEDHarpString(TOUCH_IN_0, SAMPLE_PERIOD, 4),
  LEDHarpString(TOUCH_IN_0, SAMPLE_PERIOD, 5),
  LEDHarpString(TOUCH_IN_0, SAMPLE_PERIOD, 6),
  LEDHarpString(TOUCH_IN_0, SAMPLE_PERIOD, 7),
  LEDHarpString(TOUCH_IN_0, SAMPLE_PERIOD, 8),
  LEDHarpString(TOUCH_IN_0, SAMPLE_PERIOD, 9),
  LEDHarpString(TOUCH_IN_0, SAMPLE_PERIOD, 10),
  LEDHarpString(TOUCH_IN_0, SAMPLE_PERIOD, 11),
  LEDHarpString(TOUCH_IN_0, SAMPLE_PERIOD, 12),

  LEDHarpString(TOUCH_IN_1, SAMPLE_PERIOD, 0),
  LEDHarpString(TOUCH_IN_1, SAMPLE_PERIOD, 1),
  LEDHarpString(TOUCH_IN_1, SAMPLE_PERIOD, 2),
  LEDHarpString(TOUCH_IN_1, SAMPLE_PERIOD, 3),
  LEDHarpString(TOUCH_IN_1, SAMPLE_PERIOD, 4),
  LEDHarpString(TOUCH_IN_1, SAMPLE_PERIOD, 5),
  LEDHarpString(TOUCH_IN_1, SAMPLE_PERIOD, 6),
  LEDHarpString(TOUCH_IN_1, SAMPLE_PERIOD, 7),
  LEDHarpString(TOUCH_IN_1, SAMPLE_PERIOD, 8),
  LEDHarpString(TOUCH_IN_1, SAMPLE_PERIOD, 9),
  LEDHarpString(TOUCH_IN_1, SAMPLE_PERIOD, 10),
  LEDHarpString(TOUCH_IN_1, SAMPLE_PERIOD, 11),
  LEDHarpString(TOUCH_IN_1, SAMPLE_PERIOD, 12),

  LEDHarpString(TOUCH_IN_2, SAMPLE_PERIOD, 0),
  LEDHarpString(TOUCH_IN_2, SAMPLE_PERIOD, 1),
  LEDHarpString(TOUCH_IN_2, SAMPLE_PERIOD, 2),
  LEDHarpString(TOUCH_IN_2, SAMPLE_PERIOD, 3),
  LEDHarpString(TOUCH_IN_2, SAMPLE_PERIOD, 4),
  LEDHarpString(TOUCH_IN_2, SAMPLE_PERIOD, 5),
  LEDHarpString(TOUCH_IN_2, SAMPLE_PERIOD, 6),
  LEDHarpString(TOUCH_IN_2, SAMPLE_PERIOD, 7),
  LEDHarpString(TOUCH_IN_2, SAMPLE_PERIOD, 8),
  LEDHarpString(TOUCH_IN_2, SAMPLE_PERIOD, 9),
  LEDHarpString(TOUCH_IN_2, SAMPLE_PERIOD, 10),
  LEDHarpString(TOUCH_IN_2, SAMPLE_PERIOD, 11),
  LEDHarpString(TOUCH_IN_2, SAMPLE_PERIOD, 12),
};


void update_leds() {
  // Turn on LED0 if any string is being touched
  for (int i = 0; i < NUM_STRINGS; i++) {
    if (strings[i].touching()) {
      digitalWrite(LED0,  HIGH);
      return;
    }
  }
  digitalWrite(LED0,  LOW);
}


unsigned int get_sensitivity() {
  // Read the touch sensitivity potentiometer and adjust
  // the touch sensitivity.
  unsigned int sens_raw = analogRead(SENS_IN);
  unsigned int sens = map(sens_raw, 0, 1023, 4000, 8000);
  return sens;
}


unsigned long get_duration() {
  // Read the duration input potentiometer and map
  // it to a value from 0 to 5000 milliseconds.
  unsigned int dur_raw = analogRead(DUR_IN);
  unsigned long duration = map(dur_raw, 0, 1023, 0, 5000);
  return duration;
}


// --- Globals, etc

// Define the notes that sound when each string is touched
// or plucked. For now we have pentatonic, whole tone, and
// octatonic scales, but there's no way to change them while
// playing. Need to fix that...

unsigned int pentatonic_notes[NUM_STRINGS] = {
  0,  2,  4,  7,  9,
  12, 14, 16, 19, 21,
  24, 26, 28, 31, 33,
  36, 38, 40, 43, 45,
  48, 50, 52, 55, 57,
  60, 62, 64, 67, 69,
  72, 74, 76, 79, 81,
  84, 86, 88, 91  
};

unsigned int whole_tone_notes[NUM_STRINGS] = {
  0,  2,  4,  6,  8,  10,
  12, 14, 16, 18, 20, 22,
  24, 26, 28, 30, 32, 34,
  36, 38, 40, 42, 44, 46,
  48, 50, 52, 54, 56, 58,
  60, 62, 64, 66, 68, 70,
  72, 74, 76
};

unsigned int octatonic_notes[NUM_STRINGS] = {
  0,  1,  3,  4,  6,  7,  9, 10,
  12, 13, 15, 16, 18, 19, 21, 22,
  24, 25, 27, 28, 30, 31, 33, 34,
  36, 37, 39, 40, 42, 43, 45, 46,
  48, 49, 51, 52, 54, 55, 57
};

unsigned int phrygian_dominant_notes[NUM_STRINGS] = {
  // 0  +1  +3  +1  +2  +1   +2
  0,  1,  4,  5,  7,  8,  10,
  12, 13, 16, 17, 19, 20, 22,
  24, 25, 28, 29, 31, 32, 34,
  36, 37, 40, 41, 43, 44, 46,
  48, 49, 52, 53, 55, 56, 58,
  60, 61, 64, 65
};



unsigned int *notes = phrygian_dominant_notes;


void setup() {
  // Set LED output pin
  pinMode(LED0, OUTPUT);

  // Select pins for multiplexer
  pinMode(SEL0, OUTPUT);
  pinMode(SEL1, OUTPUT);
  pinMode(SEL2, OUTPUT);
  pinMode(SEL3, OUTPUT);

  // Input pin for sensitivity adjustment
  pinMode(SENS_IN, INPUT);
  // Input pin for duration adjustment
  pinMode(DUR_IN, INPUT);
  Serial.begin(9600);

  for (int i = 0; i < NUM_STRINGS; i++) {
    strings[i].set_midi_note(notes[i] + 62);
    strings[i].set_touch_threshold(0);
    strings[i].set_hue(map(i, 0, 13, 0, 255));
  }

  // Initialize per-string LEDs
  FastLED.addLeds<NEOPIXEL, DATA_PIN, RGB>(leds, NUM_LEDS);
}


void loop() {
  // Reset touch threshold
  unsigned int sens = get_sensitivity();
  unsigned long duration = get_duration();
  for (int i = 0; i < 13; i++) {
    strings[i].set_touch_threshold(sens);
    strings[i].set_duration(duration);
    strings[i].update();
  }
  update_leds();
  FastLED.show();
}

