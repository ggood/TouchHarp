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
 
#include "HarpString.h"

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

// Sensitivity adjustment input pin
#define SENS_IN A0
// Duration adjustment input pin
#define DUR_IN A1
// Sample period - sample input no more often than this (in milliseconds)
#define SAMPLE_PERIOD 1

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
  LEDHarpString(int pin, unsigned long sample_period, unsigned int select_line, unsigned int led_index) :
  HarpString(pin, sample_period, select_line) {
    _led_index = led_index;
  };
  void set_hue(byte);
  virtual void note_on();
  virtual void note_off();
  virtual void update();
private:
  unsigned long _note_on_time = 0UL;
  unsigned int _fade_time = 2000;
  byte _hue = 200;
  unsigned int _led_index = 0;
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
    Serial.println(_led_index);
    leds[_led_index] = CHSV(_hue, 255, value);
  } else {
    leds[_led_index] = CHSV(_hue, 255, 0);
  }
}



// The current prototype has NUM_STRINGS "strings", all attached to
// analog input 9 on the Teensy
LEDHarpString strings[39] = {
  LEDHarpString(TOUCH_IN_0, SAMPLE_PERIOD, 0, 0),
  LEDHarpString(TOUCH_IN_0, SAMPLE_PERIOD, 1, 1),
  LEDHarpString(TOUCH_IN_0, SAMPLE_PERIOD, 2, 2),
  LEDHarpString(TOUCH_IN_0, SAMPLE_PERIOD, 3, 3),
  LEDHarpString(TOUCH_IN_0, SAMPLE_PERIOD, 4, 4),
  LEDHarpString(TOUCH_IN_0, SAMPLE_PERIOD, 5, 5),
  LEDHarpString(TOUCH_IN_0, SAMPLE_PERIOD, 6, 6),
  LEDHarpString(TOUCH_IN_0, SAMPLE_PERIOD, 7, 7),
  LEDHarpString(TOUCH_IN_0, SAMPLE_PERIOD, 8, 8),
  LEDHarpString(TOUCH_IN_0, SAMPLE_PERIOD, 9, 9),
  LEDHarpString(TOUCH_IN_0, SAMPLE_PERIOD, 10, 10),
  LEDHarpString(TOUCH_IN_0, SAMPLE_PERIOD, 11, 11),
  LEDHarpString(TOUCH_IN_0, SAMPLE_PERIOD, 12, 12),

  LEDHarpString(TOUCH_IN_1, SAMPLE_PERIOD, 0, 13),
  LEDHarpString(TOUCH_IN_1, SAMPLE_PERIOD, 1, 14),
  LEDHarpString(TOUCH_IN_1, SAMPLE_PERIOD, 2, 15),
  LEDHarpString(TOUCH_IN_1, SAMPLE_PERIOD, 3, 16),
  LEDHarpString(TOUCH_IN_1, SAMPLE_PERIOD, 4, 17),
  LEDHarpString(TOUCH_IN_1, SAMPLE_PERIOD, 5, 18),
  LEDHarpString(TOUCH_IN_1, SAMPLE_PERIOD, 6, 19),
  LEDHarpString(TOUCH_IN_1, SAMPLE_PERIOD, 7, 20),
  LEDHarpString(TOUCH_IN_1, SAMPLE_PERIOD, 8, 21),
  LEDHarpString(TOUCH_IN_1, SAMPLE_PERIOD, 9, 22),
  LEDHarpString(TOUCH_IN_1, SAMPLE_PERIOD, 10, 23),
  LEDHarpString(TOUCH_IN_1, SAMPLE_PERIOD, 11, 24),
  LEDHarpString(TOUCH_IN_1, SAMPLE_PERIOD, 12, 25),

  LEDHarpString(TOUCH_IN_2, SAMPLE_PERIOD, 0, 26),
  LEDHarpString(TOUCH_IN_2, SAMPLE_PERIOD, 1, 27),
  LEDHarpString(TOUCH_IN_2, SAMPLE_PERIOD, 2, 28),
  LEDHarpString(TOUCH_IN_2, SAMPLE_PERIOD, 3, 29),
  LEDHarpString(TOUCH_IN_2, SAMPLE_PERIOD, 4, 30),
  LEDHarpString(TOUCH_IN_2, SAMPLE_PERIOD, 5, 31),
  LEDHarpString(TOUCH_IN_2, SAMPLE_PERIOD, 6, 32),
  LEDHarpString(TOUCH_IN_2, SAMPLE_PERIOD, 7, 33),
  LEDHarpString(TOUCH_IN_2, SAMPLE_PERIOD, 8, 34),
  LEDHarpString(TOUCH_IN_2, SAMPLE_PERIOD, 9, 35),
  LEDHarpString(TOUCH_IN_2, SAMPLE_PERIOD, 10, 36),
  LEDHarpString(TOUCH_IN_2, SAMPLE_PERIOD, 11, 37),
  LEDHarpString(TOUCH_IN_2, SAMPLE_PERIOD, 12, 38),
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

