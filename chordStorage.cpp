/*
 * Chord Storage Implementation
 * 
 * EEPROM format starting at given offset:
 * - Magic number (4 bytes): 0x43484F52 ("CHOR")
 * - Modifier mask (4 bytes): 32-bit modifier key bitmask
 * - Chord count (4 bytes): Number of chord patterns
 * - Chord data: [32-bit keymask][null-terminated UTF-8+ string] for each chord
 * - End marker: Two null bytes (0x00 0x00)
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
    struct ChordCounter {
        uint32_t count;
        static void countCallback(uint32_t keyMask, const char* macro) {
            // Static callback needs access to the counter instance
            // We'll use a global for this simple case
        }
    };
    
    // Use a simpler approach: write chord count placeholder, then update it
    uint16_t chordCountOffset = offset;
    offset = write32ToEEPROM(offset, 0);  // Placeholder for count
    
    // Write each chord and count them
    uint32_t chordCount = 0;
    
    // We need to capture the count, so we'll use a lambda-like approach
    // Since we can't use actual lambdas, we'll use a global counter
    static uint32_t globalChordCount;
    static uint16_t globalOffset;
    globalChordCount = 0;
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
    EEPROM.write(offset++, 0x00);
    EEPROM.write(offset++, 0x00);
    
    return offset;
}

uint32_t loadChords(uint16_t startOffset, 
                   bool (*addChord)(uint32_t keyMask, const char* macroSequence),
                   void (*clearAllChords)()) {
    
    if (!addChord || !clearAllChords) return 0;
    
    uint16_t offset = startOffset;
    
    // Check magic number
    uint32_t magic;
    offset = read32FromEEPROM(offset, &magic);
    if (magic != CHORD_MAGIC_VALUE) {
        return 0;  // No valid chord data found
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
    
    // Clear existing chords before loading
    clearAllChords();
    
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
    
    // Final sanity check: did we load the expected number of chords?
    if (chordsLoaded != chordCount) {
        // Some chords failed to load, but we'll return the modifier mask anyway
        // since some chords did load successfully
    }
    
    return modifierMask;
}