/*
 * Chord Storage Interface
 * 
 * Manages persistent storage of chord patterns and modifier key configuration
 * Integrates with existing switch macro storage system
 */

#ifndef CHORD_STORAGE_H
#define CHORD_STORAGE_H

#include <Arduino.h>
#include "config.h"

//==============================================================================
// CHORD STORAGE CONFIGURATION
//==============================================================================

#define CHORD_MAGIC_VALUE 0x43484F52  // "CHOR" in hex

//==============================================================================
// CHORD STORAGE INTERFACE
//==============================================================================

// Save chords and modifier configuration to EEPROM starting at given offset
// Returns new offset after chord data for future use
uint16_t saveChords(uint16_t startOffset, uint32_t modifierMask, 
                   void (*forEachChord)(void (*callback)(uint32_t keyMask, const char* macro)));

// Load chords and modifier configuration from EEPROM starting at given offset
// Returns loaded modifier mask, updates chord system via addChord callback
// Returns 0 if no valid chord data found
uint32_t loadChords(uint16_t startOffset, 
                   bool (*addChord)(uint32_t keyMask, const char* macroSequence),
                   void (*clearAllChords)());

#endif // CHORD_STORAGE_H