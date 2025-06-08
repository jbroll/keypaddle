/*
 * Chording Keyboard Interface
 * 
 * Adds chord recognition and execution on top of existing switch system
 * Integrates with existing storage system for persistence
 */

#ifndef CHORDING_H
#define CHORDING_H

#include <Arduino.h>
#include "config.h"

//==============================================================================
// CHORDING CONFIGURATION
//==============================================================================

#define CHORD_TIMEOUT_MS 50       // Time to wait for additional keys

//==============================================================================
// CHORD PATTERN STRUCTURE (Dynamic Storage)
//==============================================================================

struct ChordPattern {
  uint32_t keyMask;              // Bitmask of keys in this chord
  char* macroSequence;           // UTF-8+ macro to execute (malloc'd)
  ChordPattern* next;            // Linked list for dynamic storage
};

//==============================================================================
// CHORDING STATE
//==============================================================================

enum ChordingState {
  CHORD_IDLE,                    // No keys pressed
  CHORD_BUILDING,                // Keys pressed, waiting for more or timeout
  CHORD_MATCHED,                 // Valid chord found and executed
  CHORD_PASSTHROUGH              // No chord match, pass to individual key handling
};

//==============================================================================
// CHORDING ENGINE CLASS
//==============================================================================

class ChordingEngine {
private:
  // Dynamic chord storage (linked list)
  ChordPattern* chordList;
  
  // Modifier key configuration (stored in EEPROM with other settings)
  uint32_t modifierKeyMask;
  
  // Current chord state (runtime only)
  uint32_t currentChord;         // Current combination of pressed keys
  uint32_t lastSwitchState;      // Previous switch state for change detection
  uint32_t chordStartTime;       // When current chord building started
  ChordingState state;
  bool chordExecuted;            // Prevent double execution
  
  // Helper methods
  ChordPattern* findChordPattern(uint32_t keyMask);
  void executeChord(ChordPattern* pattern);
  void resetChordState();
  uint8_t countBits(uint32_t mask);
  uint32_t getNonModifierKeys(uint32_t keyMask);
  bool shouldTriggerChord(uint32_t currentKeys, uint32_t previousKeys);
  void freeChordPattern(ChordPattern* pattern);
  
public:
  ChordingEngine();
  ~ChordingEngine();
  
  // Main processing function - call from main loop
  bool processChording(uint32_t currentSwitchState);
  
  // Chord management (dynamic allocation)
  bool addChord(uint32_t keyMask, const char* macroSequence);
  bool removeChord(uint32_t keyMask);
  void clearAllChords();
  
  // Modifier key management
  bool setModifierKey(uint8_t keyIndex, bool isModifier);
  bool isModifierKey(uint8_t keyIndex);
  void clearAllModifiers();
  uint32_t getModifierMask() const { return modifierKeyMask; }
  
  // Storage integration
  bool saveChords();             // Save chords to EEPROM
  bool loadChords();             // Load chords from EEPROM
  
  // Query functions
  int getChordCount();
  bool isChordDefined(uint32_t keyMask);
  const char* getChordMacro(uint32_t keyMask);
  
  // State queries
  ChordingState getCurrentState() const { return state; }
  uint32_t getCurrentChord() const { return currentChord; }
  bool isChordInProgress() const { return state == CHORD_BUILDING; }
  
  // Iteration support for commands
  void forEachChord(void (*callback)(uint32_t keyMask, const char* macro));
};

//==============================================================================
// GLOBAL INTERFACE
//==============================================================================

extern ChordingEngine chording;

// Setup function
void setupChording();

// Main processing function - returns true if chord was handled
bool processChording(uint32_t currentSwitchState);

// Chord pattern parsing helpers
uint32_t parseKeyList(const char* keyList);  // "0,1,5" -> bitmask
String formatKeyMask(uint32_t keyMask);      // bitmask -> "0+1+5"

#endif // CHORDING_H