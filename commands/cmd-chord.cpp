/*
 * CHORD Command Implementation - Updated with Storage Integration
 * 
 * Manages chord patterns and modifier keys for the chording keyboard system
 * Integrated with unified storage system
 */

#include "../serial-interface.h"
#include "../chording.h"
#include "../storage.h"
#include "../chordStorage.h"

//==============================================================================
// CHORD COMMAND IMPLEMENTATION
//==============================================================================

void cmdChord(const char* args) {
  while (isspace(*args)) args++;
  
  if (strncasecmp(args, "ADD", 3) == 0) {
    args += 3;
    while (isspace(*args)) args++;
    
    // Parse: CHORD ADD 1,2,5 "macro sequence"
    // Find space before macro
    const char* spacePos = strchr(args, ' ');
    if (!spacePos) {
      Serial.println(F("Usage: CHORD ADD <keys> <macro>"));
      return;
    }
    
    // Extract key list
    size_t keyListLen = spacePos - args;
    char keyList[32];
    if (keyListLen >= sizeof(keyList)) {
      Serial.println(F("Key list too long"));
      return;
    }
    strncpy(keyList, args, keyListLen);
    keyList[keyListLen] = '\0';
    
    // Parse key mask
    uint32_t keyMask = parseKeyList(keyList);
    if (keyMask == 0) {
      Serial.println(F("Invalid key list"));
      return;
    }
    
    // Check for duplicate chord pattern
    if (chording.isChordDefined(keyMask)) {
      Serial.println(F("Chord pattern already defined - use CHORD REMOVE first"));
      return;
    }
    
    // Check for minimum chord requirement (at least 1 non-modifier key)
    uint32_t nonModifierKeys = keyMask & ~chording.getModifierMask();
    if (nonModifierKeys == 0) {
      Serial.println(F("Chord must have at least 1 non-modifier key"));
      return;
    }
    
    // Get macro sequence
    const char* macroSeq = spacePos + 1;
    while (isspace(*macroSeq)) macroSeq++;
    
    if (*macroSeq == '\0') {
      Serial.println(F("Missing macro sequence"));
      return;
    }
    
    // Encode the macro
    MacroEncodeResult parsed = macroEncode(macroSeq);
    if (parsed.error != nullptr) {
      Serial.print(F("Parse error: "));
      Serial.println(parsed.error);
      return;
    }
    
    // Add the chord
    if (chording.addChord(keyMask, parsed.utf8Sequence)) {
      Serial.print(F("Chord "));
      Serial.print(formatKeyMask(keyMask));
      Serial.println(F(" added"));
    } else {
      Serial.println(F("Failed to add chord"));
    }
    
    // Clean up
    free(parsed.utf8Sequence);
  }
  else if (strncasecmp(args, "REMOVE", 6) == 0) {
    args += 6;
    while (isspace(*args)) args++;
    
    // Parse key list
    uint32_t keyMask = parseKeyList(args);
    if (keyMask == 0) {
      Serial.println(F("Invalid key list"));
      return;
    }
    
    if (chording.removeChord(keyMask)) {
      Serial.print(F("Chord "));
      Serial.print(formatKeyMask(keyMask));
      Serial.println(F(" removed"));
    } else {
      Serial.println(F("Chord not found"));
    }
  }
  else if (strncasecmp(args, "LIST", 4) == 0) {
    Serial.print(F("Defined chords: "));
    Serial.println(chording.getChordCount());
    Serial.println();
    
    // Print each chord
    chording.forEachChord([](uint32_t keyMask, const char* macro) {
      Serial.print(F("  "));
      Serial.print(formatKeyMask(keyMask));
      Serial.print(F(": "));
      
      // Decode and display the macro
      String readable = macroDecode((const uint8_t*)macro, strlen(macro));
      Serial.println(readable);
    });
    
    if (chording.getChordCount() == 0) {
      Serial.println(F("  (no chords defined)"));
    }
  }
  else if (strncasecmp(args, "CLEAR", 5) == 0) {
    chording.clearAllChords();
    Serial.println(F("All chords cleared"));
  }
  else if (strncasecmp(args, "MODIFIERS", 9) == 0) {
    args += 9;
    while (isspace(*args)) args++;
    
    if (strncasecmp(args, "CLEAR", 5) == 0) {
      chording.clearAllModifiers();
      Serial.println(F("All modifier keys cleared"));
    }
    else if (*args == '\0') {
      // List current modifiers
      Serial.print(F("Modifier keys: "));
      bool first = true;
      for (int i = 0; i < NUM_SWITCHES; i++) {
        if (chording.isModifierKey(i)) {
          if (!first) Serial.print(F(", "));
          Serial.print(i);
          first = false;
        }
      }
      if (first) {
        Serial.print(F("none"));
      }
      Serial.println();
    }
    else {
      // Set modifier keys from list
      uint32_t modifierMask = parseKeyList(args);
      if (modifierMask == 0 && *args != '0') {
        Serial.println(F("Invalid modifier key list"));
        return;
      }
      
      // Clear all modifiers first
      chording.clearAllModifiers();
      
      // Set new modifiers
      for (int i = 0; i < NUM_SWITCHES; i++) {
        if (modifierMask & (1UL << i)) {
          chording.setModifierKey(i, true);
        }
      }
      
      Serial.print(F("Modifier keys set to: "));
      Serial.println(formatKeyMask(modifierMask));
    }
  }
  else if (strncasecmp(args, "STATUS", 6) == 0) {
    
    if (chording.getCurrentChord() != 0) {
      Serial.print(F("Current chord: "));
      Serial.println(formatKeyMask(chording.getCurrentChord()));
    }
    
    Serial.print(F("Total chords: "));
    Serial.println(chording.getChordCount());
    
    Serial.print(F("Modifier keys: "));
    Serial.println(formatKeyMask(chording.getModifierMask()));
  }
  else {
    Serial.println(F("Usage:"));
    Serial.println(F("  CHORD ADD <keys> <macro>       - Add chord pattern"));
    Serial.println(F("  CHORD REMOVE <keys>            - Remove chord"));
    Serial.println(F("  CHORD LIST                     - List all chords"));
    Serial.println(F("  CHORD CLEAR                    - Clear all chords"));
    Serial.println(F("  CHORD MODIFIERS [keys]         - Set/show modifier keys"));
    Serial.println(F("  CHORD MODIFIERS CLEAR          - Clear all modifiers"));
    Serial.println(F("  CHORD STATUS                   - Show chording status"));
    Serial.println(F(""));
    Serial.println(F("Examples:"));
    Serial.println(F("  CHORD ADD 0,1 \"hello\"          - Keys 0+1 types hello"));
    Serial.println(F("  CHORD ADD 2+3+4 CTRL C         - Keys 2+3+4 sends Ctrl+C"));
    Serial.println(F("  CHORD MODIFIERS 1,6             - Set keys 1&6 as modifiers"));
    Serial.println(F("  CHORD REMOVE 0,1               - Remove 0+1 chord"));
  }
}

void cmdChordHelp() {
  Serial.println(F("CHORD <subcmd> - manage chord patterns and modifiers"));
}
