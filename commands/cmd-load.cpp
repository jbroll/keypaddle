/*
 * LOAD Command Implementation - Updated with Chord Storage
 * 
 * Loads both switch macros and chord configuration from EEPROM
 */

#include "../serial-interface.h"
#include "../storage.h"
#include "../chordStorage.h"
#include "../chording.h"

void cmdLoad() {
  // Load switch macros first, get end offset
  uint16_t chordOffset = loadFromStorage();
  if (chordOffset == 0) {
    Serial.println(F("Switch macro load failed"));
    return;
  }
  
  // Load chords starting after switch macros
  uint32_t modifierMask = loadChords(chordOffset,
                                    [](uint32_t keyMask, const char* macroSequence) -> bool {
                                      return chording.addChord(keyMask, macroSequence);
                                    },
                                    []() {
                                      chording.clearAllChords();
                                    });
  
  if (modifierMask > 0 || chording.getChordCount() >= 0) {
    // Update modifier mask in chording system
    chording.clearAllModifiers();
    for (int i = 0; i < NUM_SWITCHES; i++) {
      if (modifierMask & (1UL << i)) {
        chording.setModifierKey(i, true);
      }
    }
    
    Serial.println(F("Loaded"));
  } else {
    Serial.println(F("No chord data found (switch macros loaded)"));
  }
}