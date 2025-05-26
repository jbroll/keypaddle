/*
 * SAVE Command Implementation
 * 
 * Saves macros to EEPROM storage
 */

#include "../serial-interface.h"

void cmdSave() {
  if (saveToStorage()) {
    Serial.println(F("Saved"));
  } else {
    Serial.println(F("Save failed"));
  }
}