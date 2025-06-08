/*
 * Chording Keyboard Implementation with Storage Integration
 * 
 * Uses dynamic allocation and integrates with existing EEPROM storage
 */

#include "chording.h"
#include "macro-engine.h"
#include "storage.h"
#include <EEPROM.h>
#include <string.h>

//==============================================================================
// GLOBAL INSTANCE
//==============================================================================

ChordingEngine chording;

//==============================================================================
// EEPROM STORAGE LAYOUT EXTENSION
//==============================================================================

// Extend existing EEPROM layout after switch macros
// Layout: [Magic][SwitchMacros][ChordMagic][ModifierMask][ChordCount][ChordData...]

#define CHORD_MAGIC_VALUE 0xCCCC
#define CHORD_EEPROM_START (EEPROM_DATA_START + (NUM_SWITCHES * 2 * 128))  // After switch macros (estimate)

//==============================================================================
// CHORDING ENGINE IMPLEMENTATION
//==============================================================================

ChordingEngine::ChordingEngine() {
  chordList = nullptr;
  modifierKeyMask = 0;
  
  // Initialize runtime state
  currentChord = 0;
  lastSwitchState = 0;
  chordStartTime = 0;
  state = CHORD_IDLE;
  chordExecuted = false;
}

ChordingEngine::~ChordingEngine() {
  clearAllChords();
}

bool ChordingEngine::processChording(uint32_t currentSwitchState) {
  uint32_t now = millis();
  uint32_t pressed = currentSwitchState & ~lastSwitchState;   // Newly pressed
  uint32_t released = lastSwitchState & ~currentSwitchState;  // Newly released
  
  switch (state) {
    case CHORD_IDLE:
      if (pressed) {
        // Start building a chord
        currentChord = currentSwitchState;
        chordStartTime = now;
        state = CHORD_BUILDING;
        chordExecuted = false;
      }
      break;
      
    case CHORD_BUILDING:
      if (pressed) {
        // More keys pressed - update chord
        currentChord = currentSwitchState;
        chordStartTime = now;  // Reset timeout
      }
      else if (released) {
        // Keys released while building - check if we should trigger
        if (shouldTriggerChord(currentSwitchState, lastSwitchState)) {
          // Non-modifier keys released - try to match and execute chord
          ChordPattern* pattern = findChordPattern(currentChord);
          if (pattern) {
            executeChord(pattern);
            state = CHORD_MATCHED;
            lastSwitchState = currentSwitchState;
            return true;  // Chord handled - don't process individual keys
          } else {
            // No chord match - pass through to individual key processing
            state = CHORD_PASSTHROUGH;
            lastSwitchState = currentSwitchState;
            return false;  // Let individual keys be processed
          }
        } else {
          // Only modifier keys or some non-modifiers still pressed - continue building
          currentChord = currentSwitchState;
        }
      }
      else if (now - chordStartTime > CHORD_TIMEOUT_MS) {
        // Timeout - no more keys expected, try to match current combination
        if (currentSwitchState > 0) {
          ChordPattern* pattern = findChordPattern(currentChord);
          if (pattern) {
            // Found a partial match - wait for non-modifier release to execute
            // Keep building until non-modifier keys released
          } else {
            // No match possible - pass through
            state = CHORD_PASSTHROUGH;
            return false;
          }
        }
      }
      break;
      
    case CHORD_MATCHED:
      if (getNonModifierKeys(currentSwitchState) == 0) {
        // All non-modifier keys released after successful chord
        resetChordState();
      }
      // Suppress individual key processing until non-modifier keys released
      lastSwitchState = currentSwitchState;
      return true;
      
    case CHORD_PASSTHROUGH:
      if (getNonModifierKeys(currentSwitchState) == 0) {
        // All non-modifier keys released - return to idle
        resetChordState();
      }
      // Let individual keys be processed normally
      break;
  }
  
  lastSwitchState = currentSwitchState;
  return false;  // Continue with individual key processing
}

ChordPattern* ChordingEngine::findChordPattern(uint32_t keyMask) {
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
  chordExecuted = true;
}

void ChordingEngine::resetChordState() {
  currentChord = 0;
  chordStartTime = 0;
  state = CHORD_IDLE;
  chordExecuted = false;
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
}

void ChordingEngine::freeChordPattern(ChordPattern* pattern) {
  if (pattern) {
    if (pattern->macroSequence) {
      free(pattern->macroSequence);
    }
    free(pattern);
  }
}

//==============================================================================
// STORAGE INTEGRATION
//==============================================================================

bool ChordingEngine::saveChords() {
  // Find end of switch macro data to place chord data
  uint16_t offset = CHORD_EEPROM_START;
  
  // Write chord magic number
  uint32_t magic = CHORD_MAGIC_VALUE;
  EEPROM.put(offset, magic);
  offset += sizeof(magic);
  
  // Write modifier mask
  EEPROM.put(offset, modifierKeyMask);
  offset += sizeof(modifierKeyMask);
  
  // Count chords
  uint16_t chordCount = getChordCount();
  EEPROM.put(offset, chordCount);
  offset += sizeof(chordCount);
  
  // Write each chord
  ChordPattern* current = chordList;
  while (current && offset < EEPROM.length()) {
    // Write key mask
    EEPROM.put(offset, current->keyMask);
    offset += sizeof(current->keyMask);
    
    // Write macro length
    uint16_t macroLen = strlen(current->macroSequence);
    EEPROM.put(offset, macroLen);
    offset += sizeof(macroLen);
    
    // Write macro data
    for (uint16_t i = 0; i <= macroLen && offset < EEPROM.length(); i++) {
      EEPROM.write(offset++, current->macroSequence[i]);
    }
    
    current = current->next;
  }
  
  return true;
}

bool ChordingEngine::loadChords() {
  uint16_t offset = CHORD_EEPROM_START;
  
  // Check magic number
  uint32_t magic;
  EEPROM.get(offset, magic);
  if (magic != CHORD_MAGIC_VALUE) {
    return false;  // No chord data found
  }
  offset += sizeof(magic);
  
  // Clear existing chords
  clearAllChords();
  
  // Load modifier mask
  EEPROM.get(offset, modifierKeyMask);
  offset += sizeof(modifierKeyMask);
  
  // Load chord count
  uint16_t chordCount;
  EEPROM.get(offset, chordCount);
  offset += sizeof(chordCount);
  
  // Load each chord
  for (uint16_t i = 0; i < chordCount && offset < EEPROM.length(); i++) {
    // Read key mask
    uint32_t keyMask;
    EEPROM.get(offset, keyMask);
    offset += sizeof(keyMask);
    
    // Read macro length
    uint16_t macroLen;
    EEPROM.get(offset, macroLen);
    offset += sizeof(macroLen);
    
    // Validate length
    if (macroLen > 512 || offset + macroLen >= EEPROM.length()) {
      break;  // Invalid data
    }
    
    // Read macro data
    char* macroBuffer = (char*)malloc(macroLen + 1);
    if (!macroBuffer) break;
    
    for (uint16_t j = 0; j <= macroLen; j++) {
      macroBuffer[j] = EEPROM.read(offset++);
    }
    
    // Add chord to list
    addChord(keyMask, macroBuffer);
    free(macroBuffer);
  }
  
  return true;
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

bool ChordingEngine::isModifierKey(uint8_t keyIndex) {
  if (keyIndex >= NUM_SWITCHES) return false;
  return (modifierKeyMask & (1UL << keyIndex)) != 0;
}

void ChordingEngine::clearAllModifiers() {
  modifierKeyMask = 0;
}

uint32_t ChordingEngine::getNonModifierKeys(uint32_t keyMask) {
  return keyMask & ~modifierKeyMask;
}

bool ChordingEngine::shouldTriggerChord(uint32_t currentKeys, uint32_t previousKeys) {
  // Trigger when any non-modifier key is released
  uint32_t prevNonMod = getNonModifierKeys(previousKeys);
  uint32_t currNonMod = getNonModifierKeys(currentKeys);
  
  // If any non-modifier key was released, trigger chord evaluation
  return (prevNonMod & ~currNonMod) != 0;
}

//==============================================================================
// QUERY FUNCTIONS
//==============================================================================

int ChordingEngine::getChordCount() {
  int count = 0;
  ChordPattern* current = chordList;
  while (current) {
    count++;
    current = current->next;
  }
  return count;
}

bool ChordingEngine::isChordDefined(uint32_t keyMask) {
  return findChordPattern(keyMask) != nullptr;
}

const char* ChordingEngine::getChordMacro(uint32_t keyMask) {
  ChordPattern* pattern = findChordPattern(keyMask);
  return pattern ? pattern->macroSequence : nullptr;
}

void ChordingEngine::forEachChord(void (*callback)(uint32_t keyMask, const char* macro)) {
  ChordPattern* current = chordList;
  while (current) {
    callback(current->keyMask, current->macroSequence);
    current = current->next;
  }
}

uint8_t ChordingEngine::countBits(uint32_t mask) {
  uint8_t count = 0;
  while (mask) {
    count += mask & 1;
    mask >>= 1;
  }
  return count;
}

//==============================================================================
// GLOBAL INTERFACE FUNCTIONS
//==============================================================================

void setupChording() {
  // Chording engine initializes itself
  // Load chords from EEPROM if available
  chording.loadChords();
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
