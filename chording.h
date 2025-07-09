/*
 * Chording Keyboard Interface - Complete Implementation
 * 
 * Features:
 * - State machine with execution windows and cancellation
 * - Conflict prevention between chord and individual switches
 * - Automatic chord pattern adjustment during release
 * - Modifier key support
 */

#ifndef CHORDING_H
#define CHORDING_H

#include <Arduino.h>
#include "config.h"

//==============================================================================
// CHORD PATTERN STRUCTURE
//==============================================================================

struct ChordPattern {
    uint32_t keyMask;              // Bitmask of keys in this chord
    char* macroSequence;           // UTF-8+ macro to execute (malloc'd)
    ChordPattern* next;            // Linked list for dynamic storage
};

//==============================================================================
// CHORD STATE ENUMERATION
//==============================================================================

enum ChordState {
    CHORD_IDLE,                    // No keys pressed, normal operation
    CHORD_BUILDING,                // Chord keys pressed, building pattern
    CHORD_CANCELLATION             // Non-chord key pressed, suppressing execution
};

//==============================================================================
// CHORDING ENGINE CLASS
//==============================================================================

class ChordingEngine {
private:
    // Chord storage
    ChordPattern* chordList;
    uint32_t modifierKeyMask;       // Which keys are modifiers
    uint32_t chordSwitchesMask;     // Bitmask of all switches used in any chord
    
    // State machine
    ChordState state;
    uint32_t capturedChord;         // Accumulated chord pattern
    uint32_t pressedKeys;           // Currently pressed switches
    uint32_t lastSwitchState;       // For change detection
    
    // Timing state
    uint32_t executionWindowMs;     // Execution window duration (default 50ms)
    uint32_t executionWindowStart; // Window start time
    bool executionWindowActive;    // Window active flag
    uint32_t cancellationStartTime; // Cancellation window start time
    
    // Helper methods
    ChordPattern* findChordPattern(uint32_t keyMask) const;
    void executeChord(ChordPattern* pattern);
    void freeChordPattern(ChordPattern* pattern);
    void updateChordSwitchesMask();
    uint32_t getNonModifierKeys(uint32_t keyMask) const;
    void handleExecutionWindow();
    void handleCancellationTimeout();
    void resetState();
    

    int lastState;
public:
    ChordingEngine();
    ~ChordingEngine();
    
    // Main processing function - call from main loop
    bool processChording(uint32_t currentSwitchState);
    
    // Chord management
    bool addChord(uint32_t keyMask, const char* macroSequence);
    bool removeChord(uint32_t keyMask);
    void clearAllChords();
    
    // Modifier key management
    bool setModifierKey(uint8_t keyIndex, bool isModifier);
    bool isModifierKey(uint8_t keyIndex) const;
    void clearAllModifiers();
    uint32_t getModifierMask() const { return modifierKeyMask; }
    
    // Configuration
    void setExecutionWindowMs(uint32_t windowMs) { executionWindowMs = windowMs; }
    uint32_t getExecutionWindowMs() const { return executionWindowMs; }
    
    // Query functions
    int getChordCount() const;
    bool isChordDefined(uint32_t keyMask) const;
    const char* getChordMacro(uint32_t keyMask) const;
    bool isSwitchUsedInChords(uint8_t switchIndex) const;
    uint32_t getChordSwitchesMask() const { return chordSwitchesMask; }
    
    // State queries
    ChordState getCurrentState() const { return state; }
    uint32_t getCurrentChord() const { return capturedChord; }
    bool isExecutionWindowActive() const { return executionWindowActive; }
    
    // Iteration support for commands and storage
    void forEachChord(void (*callback)(uint32_t keyMask, const char* macro)) const;
};

//==============================================================================
// GLOBAL INTERFACE
//==============================================================================

extern ChordingEngine chording;

// Setup function
void setupChording();

// Main processing function - returns true if individual key processing should be suppressed
bool processChording(uint32_t currentSwitchState);

// Chord pattern parsing helpers
uint32_t parseKeyList(const char* keyList);  // "0,1,5" -> bitmask
String formatKeyMask(uint32_t keyMask);      // bitmask -> "0+1+5"

#endif // CHORDING_H
