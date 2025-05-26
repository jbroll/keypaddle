/*
 * LOAD Command Implementation
 * 
 * Loads macros from EEPROM storage
 */

#include "../serial-interface.h"

void cmdLoad() {
  if (loadFromStorage()) {
    Serial.println(F("Loaded"));
  } else {
    Serial.println(F("Load failed"));
  }
}