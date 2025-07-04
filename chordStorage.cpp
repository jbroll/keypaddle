/*
 * Chord Storage Implementation - FIXED VERSION
 * 
 * This is the corrected version that should replace the existing chordStorage.cpp
 * 
 * EEPROM format starting at given offset:
 * - Magic number (4 bytes): 0x43484F52 ("CHOR")
 * - Modifier mask (4 bytes): 32-bit modifier key bitmask
 * - Chord count (4 bytes): Number of chord patterns
 * - Chord data: [32-bit keymask][null-terminated UTF-8+ string] for each chord
 * - End marker: Two null bytes (0x00 0x00)
 * 
 * KEY FIXES:
 * 1. loadChords now ALWAYS calls clearAllChords (even on empty EEPROM)
 * 2. loadChords now ALWAYS returns the modifier mask (even if 0)
 * 3. Better error handling and validation
 */

#include "chordStorage.h"
#include <EEPROM.h>

//==============================================================================
// EXTERNAL STORAGE HELPERS (from storage.cpp)
//==============================================================================

// External functions from storage.cpp
extern uint16_t writeStringToEEPROM(uint16_t offset, const char* str);
extern uint16_t readStringFromEEPROM(uint16_t offset, char** str);

//==============================================================================
// HELPER FUNCTIONS
//==============================================================================

// Write a 32-bit value to EEPROM at offset, return new offset
static uint16_t write32ToEEPROM(uint16_t offset, uint32_t value) {
    EEPROM.put(offset, value);
    return offset + sizeof(uint32_t);
}

// Read a 32-bit value from EEPROM at offset, return new offset
static uint16_t read32FromEEPROM(uint16_t offset, uint32_t* value) {
    EEPROM.get(offset, *value);
    return offset + sizeof(uint32_t);
}

//==============================================================================
// CHORD STORAGE IMPLEMENTATION
//==============================================================================

uint16_t saveChords(uint16_t startOffset, uint32_t modifierMask, 
                   void (*forEachChord)(void (*callback)(uint32_t keyMask, const char* macro))) {
    
    if (!forEachChord) return startOffset;
    
    uint16_t offset = startOffset;
    
    // Write magic number
    offset = write32ToEEPROM(offset, CHORD_MAGIC_VALUE);
    
    // Write modifier mask
    offset = write32ToEEPROM(offset, modifierMask);
    
    // Count chords first by calling forEachChord with a counting callback
    static uint32_t globalChordCount;
    static uint16_t globalOffset;
    globalChordCount = 0;
    globalOffset = offset;
    
    // Reserve space for chord count, we'll update it later
    uint16_t chordCountOffset = offset;
    offset = write32ToEEPROM(offset, 0);  // Placeholder for count
    globalOffset = offset;
    
    // Define a static callback that writes chords and counts them
    static auto writeChordCallback = [](uint32_t keyMask, const char* macro) -> void {
        if (!macro) return;
        
        // Write key mask
        globalOffset = write32ToEEPROM(globalOffset, keyMask);
        
        // Write macro string
        globalOffset = writeStringToEEPROM(globalOffset, macro);
        
        globalChordCount++;
    };
    
    // Call forEachChord with our writing callback
    forEachChord(writeChordCallback);
    
    // Update the chord count in the header
    write32ToEEPROM(chordCountOffset, globalChordCount);
    
    // Update our local offset from the global
    offset = globalOffset;
    
    // Write end marker (two null bytes)
    if (offset < EEPROM.length()) EEPROM.write(offset++, 0x00);
    if (offset < EEPROM.length()) EEPROM.write(offset++, 0x00);
    
    return offset;
}

uint32_t loadChords(uint16_t startOffset, 
                   bool (*addChord)(uint32_t keyMask, const char* macroSequence),
                   void (*clearAllChords)()) {
    
    // CRITICAL FIX #1: Always call clearAllChords, even if parameters are invalid
    if (clearAllChords) {
        clearAllChords();
    }
    
    if (!addChord || !clearAllChords) return 0;
    
    uint16_t offset = startOffset;
    
    // Check magic number
    uint32_t magic;
    offset = read32FromEEPROM(offset, &magic);
    if (magic != CHORD_MAGIC_VALUE) {
        // CRITICAL FIX #2: No valid chord data found - return 0 but clearAllChords was already called
        return 0;
    }
    
    // Read modifier mask
    uint32_t modifierMask;
    offset = read32FromEEPROM(offset, &modifierMask);
    
    // Read chord count
    uint32_t chordCount;
    offset = read32FromEEPROM(offset, &chordCount);
    
    // Sanity check: reasonable chord count (prevent runaway reads)
    if (chordCount > 1000) {  // Arbitrary but reasonable limit
        return 0;
    }
    
    // Load each chord
    uint32_t chordsLoaded = 0;
    for (uint32_t i = 0; i < chordCount && offset < EEPROM.length(); i++) {
        // Read key mask
        uint32_t keyMask;
        offset = read32FromEEPROM(offset, &keyMask);
        
        if (offset >= EEPROM.length()) break;
        
        // Read macro string
        char* macroString = nullptr;
        offset = readStringFromEEPROM(offset, &macroString);
        
        if (offset == 0) {  // Read error
            if (macroString) free(macroString);
            break;
        }
        
        // Add chord to system (ignore return value - first duplicate wins)
        if (macroString) {
            addChord(keyMask, macroString);
            free(macroString);
            chordsLoaded++;
        }
    }
    
    // Verify end marker (optional - for debugging)
    if (offset < EEPROM.length() - 1) {
        uint8_t endMarker1 = EEPROM.read(offset);
        uint8_t endMarker2 = EEPROM.read(offset + 1);
        if (endMarker1 != 0x00 || endMarker2 != 0x00) {
            // End marker mismatch - data might be corrupted
            // But we'll continue since we got some valid data
        }
    }
    
    // CRITICAL FIX #3: Always return the modifier mask (even if 0)
    return modifierMask;
}