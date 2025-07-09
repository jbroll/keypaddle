/*
 * Chording Keyboard Implementation - FIXED State Machine
 * 
 * KEY FIXES:
 * 1. CANCELLATION state persists until all keys released OR timeout
 * 2. Pattern adjustment only happens in CHORD_BUILDING state
 * 3. Execution window properly respects state
 * 4. Better state transition logic
 */

#include "chording.h"
#include "macro-engine.h"
#include "storage.h"
#include <string.h>

//==============================================================================
// CONSTANTS
//==============================================================================

static const uint32_t DEFAULT_EXECUTION_WINDOW_MS = 50;
static const uint32_t CANCELLATION_TIMEOUT_MS = 2000;

//==============================================================================
// GLOBAL INSTANCE
//==============================================================================

ChordingEngine chording;

//==============================================================================
// CHORDING ENGINE IMPLEMENTATION
//==============================================================================

ChordingEngine::ChordingEngine() {
    chordList = nullptr;
    modifierKeyMask = 0;
    chordSwitchesMask = 0;
    state = CHORD_IDLE;
    capturedChord = 0;
    pressedKeys = 0;
    lastSwitchState = 0;
    executionWindowMs = DEFAULT_EXECUTION_WINDOW_MS;
    executionWindowStart = 0;
    executionWindowActive = false;
    cancellationStartTime = 0;
}

ChordingEngine::~ChordingEngine() {
    clearAllChords();
}

bool ChordingEngine::processChording(uint32_t currentSwitchState) {
    uint32_t now = millis();
    
    // Update pressed keys state
    pressedKeys = currentSwitchState;
    
    // Calculate ALL key changes first
    uint32_t allPressed = currentSwitchState & ~lastSwitchState;
    uint32_t allReleased = lastSwitchState & ~currentSwitchState;
    
    // Separate into chord and non-chord keys
    uint32_t chordSwitches = currentSwitchState & chordSwitchesMask;
    uint32_t chordPressed = allPressed & chordSwitchesMask;
    uint32_t chordReleased = allReleased & chordSwitchesMask;
    
    uint32_t nonChordPressed = allPressed & ~chordSwitchesMask;
    
    // Handle state machine
    switch (state) {
        case CHORD_IDLE:
            if (chordPressed) {
                // Chord key pressed - start building
                state = CHORD_BUILDING;
                capturedChord = chordSwitches;
                executionWindowActive = false;
            }
            break;
            
        case CHORD_BUILDING:
            if (chordPressed) {
                // More chord keys pressed - expand pattern
                capturedChord |= chordSwitches;
            }
            
            // Check for cancellation: non-chord, non-modifier key pressed
            if (nonChordPressed) {
                uint32_t nonModifierNonChord = nonChordPressed & ~modifierKeyMask;
                if (nonModifierNonChord) {
                    state = CHORD_CANCELLATION;
                    cancellationStartTime = now;
                    executionWindowActive = false;
                }
            }
            
            if (chordReleased) {
                // Start execution window on first chord key release
                if (!executionWindowActive) {
                    executionWindowStart = now;
                    executionWindowActive = true;
                }
            }
            break;
            
        case CHORD_CANCELLATION:
            // CRITICAL FIX 1: Stay in cancellation until timeout OR all keys released
            
            // Reset cancellation timer on additional non-chord key presses
            if (nonChordPressed) {
                uint32_t nonModifierNonChord = nonChordPressed & ~modifierKeyMask;
                if (nonModifierNonChord) {
                    cancellationStartTime = now;
                }
            }
            
            if (chordReleased) {
                // Start execution window on chord key release (but won't execute in cancellation)
                if (!executionWindowActive) {
                    executionWindowStart = now;
                    executionWindowActive = true;
                }
            }
            
            // Check for cancellation timeout
            if (now - cancellationStartTime >= CANCELLATION_TIMEOUT_MS) {
                if (chordSwitches != 0) {
                    // Return to building with current chord keys
                    state = CHORD_BUILDING;
                    capturedChord = chordSwitches;
                    executionWindowActive = false;
                } else {
                    // No chord keys left - return to idle
                    resetState();
                }
            }
            
            // CRITICAL FIX 2: DON'T exit cancellation state here unless all keys released
            // This was the main bug - cancellation was exiting too early
            break;
    }
    
    // Handle execution window timeout - but only in CHORD_BUILDING state
    if (executionWindowActive && (now - executionWindowStart >= executionWindowMs)) {
        handleExecutionWindow();
    }
    
    // CRITICAL FIX 3: Handle complete key release properly
    if (pressedKeys == 0) {
        if (executionWindowActive && state == CHORD_BUILDING) {
            // All keys released within execution window AND in building state - execute chord
            ChordPattern* pattern = findChordPattern(capturedChord);
            if (pattern) {
                executeChord(pattern);
            }
        }
        // Always reset to IDLE when all keys are released, regardless of state
        resetState();
    }
    
    lastSwitchState = currentSwitchState;
    
    // Suppress individual key processing if not in idle state
    return (state != CHORD_IDLE);
}

void ChordingEngine::handleExecutionWindow() {
    if (pressedKeys == 0) {
        // All keys released - handled in main function
        return;
    }
    
    // CRITICAL FIX 4: Only update pattern if we're in CHORD_BUILDING state
    if (state == CHORD_BUILDING) {
        // Some keys still held - update pattern to currently pressed chord keys
        uint32_t currentChordKeys = pressedKeys & chordSwitchesMask;
        if (currentChordKeys != 0) {
            capturedChord = currentChordKeys;
        } else {
            // No chord keys left - should transition to idle
            resetState();
            return;
        }
    }
    
    executionWindowActive = false;
}

void ChordingEngine::resetState() {
    state = CHORD_IDLE;
    capturedChord = 0;
    executionWindowActive = false;
    cancellationStartTime = 0;
}

ChordPattern* ChordingEngine::findChordPattern(uint32_t keyMask) const {
    ChordPattern* current = chordList;
    while (current) {
        if (current->keyMask == keyMask) {
            return current;
        }
        current = current->next;
    }
    return nullptr;
}

void ChordingEngine::executeChord(ChordPattern* pattern) {
    if (!pattern || !pattern->macroSequence) return;
    
    // Execute the chord's macro sequence
    executeUTF8Macro((const uint8_t*)pattern->macroSequence, strlen(pattern->macroSequence));
}

bool ChordingEngine::addChord(uint32_t keyMask, const char* macroSequence) {
    if (!macroSequence || keyMask == 0) return false;
    
    // Check for valid chord (at least one non-modifier key)
    if (getNonModifierKeys(keyMask) == 0) return false;
    
    // Find existing pattern or create new one
    ChordPattern* pattern = findChordPattern(keyMask);
    
    if (pattern) {
        // Update existing pattern
        if (pattern->macroSequence) {
            free(pattern->macroSequence);
        }
    } else {
        // Create new pattern
        pattern = (ChordPattern*)malloc(sizeof(ChordPattern));
        if (!pattern) return false;
        
        pattern->keyMask = keyMask;
        pattern->next = chordList;
        chordList = pattern;
    }
    
    // Set macro sequence
    pattern->macroSequence = (char*)malloc(strlen(macroSequence) + 1);
    if (!pattern->macroSequence) {
        // If this was a new pattern, remove it from list
        if (pattern->next == chordList) {
            chordList = pattern->next;
            free(pattern);
        }
        return false;
    }
    
    strcpy(pattern->macroSequence, macroSequence);
    updateChordSwitchesMask();
    return true;
}

bool ChordingEngine::removeChord(uint32_t keyMask) {
    ChordPattern* current = chordList;
    ChordPattern* previous = nullptr;
    
    while (current) {
        if (current->keyMask == keyMask) {
            // Remove from linked list
            if (previous) {
                previous->next = current->next;
            } else {
                chordList = current->next;
            }
            
            // Free memory
            freeChordPattern(current);
            updateChordSwitchesMask();
            return true;
        }
        previous = current;
        current = current->next;
    }
    return false;
}

void ChordingEngine::clearAllChords() {
    while (chordList) {
        ChordPattern* next = chordList->next;
        freeChordPattern(chordList);
        chordList = next;
    }
    chordSwitchesMask = 0;
    resetState();
}

void ChordingEngine::freeChordPattern(ChordPattern* pattern) {
    if (pattern) {
        if (pattern->macroSequence) {
            free(pattern->macroSequence);
        }
        free(pattern);
    }
}

void ChordingEngine::updateChordSwitchesMask() {
    chordSwitchesMask = 0;
    ChordPattern* current = chordList;
    while (current) {
        chordSwitchesMask |= current->keyMask;
        current = current->next;
    }
}

//==============================================================================
// MODIFIER KEY MANAGEMENT
//==============================================================================

bool ChordingEngine::setModifierKey(uint8_t keyIndex, bool isModifier) {
    if (keyIndex >= NUM_SWITCHES) return false;
    
    if (isModifier) {
        modifierKeyMask |= (1UL << keyIndex);
    } else {
        modifierKeyMask &= ~(1UL << keyIndex);
    }
    return true;
}

bool ChordingEngine::isModifierKey(uint8_t keyIndex) const {
    if (keyIndex >= NUM_SWITCHES) return false;
    return (modifierKeyMask & (1UL << keyIndex)) != 0;
}

void ChordingEngine::clearAllModifiers() {
    modifierKeyMask = 0;
}

uint32_t ChordingEngine::getNonModifierKeys(uint32_t keyMask) const {
    return keyMask & ~modifierKeyMask;
}

//==============================================================================
// QUERY FUNCTIONS
//==============================================================================

int ChordingEngine::getChordCount() const {
    int count = 0;
    ChordPattern* current = chordList;
    while (current) {
        count++;
        current = current->next;
    }
    return count;
}

bool ChordingEngine::isChordDefined(uint32_t keyMask) const {
    return findChordPattern(keyMask) != nullptr;
}

const char* ChordingEngine::getChordMacro(uint32_t keyMask) const {
    ChordPattern* pattern = findChordPattern(keyMask);
    return pattern ? pattern->macroSequence : nullptr;
}

bool ChordingEngine::isSwitchUsedInChords(uint8_t switchIndex) const {
    if (switchIndex >= NUM_SWITCHES) return false;
    return (chordSwitchesMask & (1UL << switchIndex)) != 0;
}

void ChordingEngine::forEachChord(void (*callback)(uint32_t keyMask, const char* macro)) const {
    ChordPattern* current = chordList;
    while (current) {
        if (callback) {
            callback(current->keyMask, current->macroSequence);
        }
        current = current->next;
    }
}

//==============================================================================
// GLOBAL INTERFACE FUNCTIONS
//==============================================================================

void setupChording() {
    // Chording engine initializes itself
}

bool processChording(uint32_t currentSwitchState) {
    return chording.processChording(currentSwitchState);
}

//==============================================================================
// UTILITY FUNCTIONS
//==============================================================================

uint32_t parseKeyList(const char* keyList) {
    if (!keyList) return 0;
    
    uint32_t mask = 0;
    const char* pos = keyList;
    
    while (*pos) {
        // Skip whitespace and separators
        while (*pos && (*pos == ' ' || *pos == ',' || *pos == '+')) pos++;
        
        if (*pos) {
            // Parse key number
            int keyNum = 0;
            while (*pos >= '0' && *pos <= '9') {
                keyNum = keyNum * 10 + (*pos - '0');
                pos++;
            }
            
            if (keyNum >= 0 && keyNum < NUM_SWITCHES) {
                mask |= (1UL << keyNum);
            }
        }
    }
    
    return mask;
}

String formatKeyMask(uint32_t keyMask) {
    String result = "";
    bool first = true;
    
    for (int i = 0; i < NUM_SWITCHES; i++) {
        if (keyMask & (1UL << i)) {
            if (!first) result += "+";
            result += String(i);
            first = false;
        }
    }
    
    return result.length() > 0 ? result : String("none");
}