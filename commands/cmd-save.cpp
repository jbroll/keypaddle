/*
 * SAVE Command Implementation - Updated with Chord Storage
 * 
 * Saves both switch macros and chord configuration to EEPROM
 */

#include "../serial-interface.h"
#include "../storage.h"
#include "../chordStorage.h"
#include "../chording.h"

void cmdSave() {
  // Save switch macros first, get end offset
  uint16_t chordOffset = saveToStorage();
  if (chordOffset == 0) {
    Serial.println(F("Switch macro save failed"));
    return;
  }
  
  // Save chords starting after switch macros
  uint16_t finalOffset = saveChords(chordOffset, chording.getModifierMask(), 
                                   [](void (*callback)(uint32_t keyMask, const char* macro)) {
                                     chording.forEachChord(callback);
                                   });
  
  if (finalOffset > chordOffset) {
    Serial.println(F("Saved"));
  } else {
    Serial.println(F("Chord save failed"));
  }
}