/*
 * MAP Parser Shared Tables Implementation
 * 
 * Lookup tables and shared utility functions
 */

#include "map-parser-tables.h"
#include <Keyboard.h>

//==============================================================================
// KEYWORD TO HID CODE MAPPING TABLE
//==============================================================================

// Table-driven keyword to HID code mapping
// First entry for each HID code is the preferred display name
const KeywordMapping KEYWORD_TABLE[] PROGMEM = {
  // Function keys
  {"F1",        KEY_F1},
  {"F2",        KEY_F2},
  {"F3",        KEY_F3},
  {"F4",        KEY_F4},
  {"F5",        KEY_F5},
  {"F6",        KEY_F6},
  {"F7",        KEY_F7},
  {"F8",        KEY_F8},
  {"F9",        KEY_F9},
  {"F10",       KEY_F10},
  {"F11",       KEY_F11},
  {"F12",       KEY_F12},
  
  // Arrow keys
  {"UP",        KEY_UP_ARROW},
  {"DOWN",      KEY_DOWN_ARROW},
  {"LEFT",      KEY_LEFT_ARROW},
  {"RIGHT",     KEY_RIGHT_ARROW},
  
  // Navigation
  {"HOME",      KEY_HOME},
  {"END",       KEY_END},
  {"PAGEUP",    KEY_PAGE_UP},
  {"PAGEDOWN",  KEY_PAGE_DOWN},
  {"DELETE",    KEY_DELETE},
  {"DEL",       KEY_DELETE},      // Alias - will display as DELETE (first entry)
  
  // Special keys that map to our control codes
  {"ENTER",     UTF8_ENTER},
  {"TAB",       UTF8_TAB},
  {"ESC",       UTF8_ESCAPE},
  {"ESCAPE",    UTF8_ESCAPE},     // Alias - will display as ESC (first entry)
  {"BACKSPACE", UTF8_BACKSPACE},
  {"SPACE",     ' '},
};

const int KEYWORD_TABLE_SIZE = sizeof(KEYWORD_TABLE) / sizeof(KeywordMapping);

//==============================================================================
// MODIFIER LOOKUP TABLE
//==============================================================================

const ModifierInfo MODIFIERS[] PROGMEM = {
  {"CTRL",  MULTI_CTRL},
  {"ALT",   MULTI_ALT}, 
  {"SHIFT", MULTI_SHIFT},
  {"CMD",   MULTI_CMD},
  {"WIN",   MULTI_CMD},  // Alias for CMD
  {"GUI",   MULTI_CMD},  // Another alias
};

const int NUM_MODIFIERS = 6;

//==============================================================================
// SHARED LOOKUP FUNCTIONS
//==============================================================================

uint8_t findHIDCodeForKeyword(const String& keyword) {
  for (int i = 0; i < KEYWORD_TABLE_SIZE; i++) {
    KeywordMapping entry;
    memcpy_P(&entry, &KEYWORD_TABLE[i], sizeof(KeywordMapping));
    if (keyword.equalsIgnoreCase(entry.keyword)) {
      return entry.hidCode;
    }
  }
  return 0; // Not found
}

const char* findKeywordForHID(uint8_t hidCode) {
  for (int i = 0; i < KEYWORD_TABLE_SIZE; i++) {
    KeywordMapping entry;
    memcpy_P(&entry, &KEYWORD_TABLE[i], sizeof(KeywordMapping));
    if (entry.hidCode == hidCode) {
      return entry.keyword;  // First match is preferred display name
    }
  }
  return nullptr; // Not found
}

uint8_t findModifierBit(const String& name) {
  for (int i = 0; i < NUM_MODIFIERS; i++) {
    ModifierInfo mod;
    memcpy_P(&mod, &MODIFIERS[i], sizeof(ModifierInfo));
    if (name.equalsIgnoreCase(mod.name)) {
      return mod.multiBit;
    }
  }
  return 0;
}

//==============================================================================
// SHARED UTILITY FUNCTIONS
//==============================================================================

bool isRegularCharacter(uint8_t b) {
  // Regular printable characters and UTF-8 extended characters
  return (b >= 0x20 && b <= 0x7E) || (b > 0x7E);
}

bool needsQuoting(uint8_t b) {
  // Characters that should be quoted even if single
  return (b == ' ' || b == '\t' || b == '\n' || b == '\r' || 
          b == '"' || b == '\\' || b < 0x20);
}