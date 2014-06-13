#include "HarpString.h"


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

