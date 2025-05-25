/*
 * MAP Parser Shared Tables Implementation
 * 
 * Lookup tables and shared utility functions
 */

#include "map-parser-tables.h"
#include <Keyboard.h>

//==============================================================================
// KEYWORD TO UTF-8+ CODE MAPPING TABLE
//==============================================================================

// Table-driven keyword to UTF-8+ code mapping
// Function keys use special 2-byte encoding, other keys use direct codes
const KeywordMapping KEYWORD_TABLE[] PROGMEM = {
  // Function keys - these will be handled specially in encoder/decoder
  // Listed here for completeness but use 2-byte [0x05, n] encoding
  {"F1",        0x01},   // Function key number (used with UTF8_FUNCTION_KEY prefix)
  {"F2",        0x02},
  {"F3",        0x03},
  {"F4",        0x04},
  {"F5",        0x05},
  {"F6",        0x06},
  {"F7",        0x07},
  {"F8",        0x08},
  {"F9",        0x09},
  {"F10",       0x0A},
  {"F11",       0x0B},
  {"F12",       0x0C},
  
  // Arrow keys - mapped to UTF-8+ safe space (single byte)
  {"UP",        UTF8_KEY_UP},
  {"DOWN",      UTF8_KEY_DOWN},
  {"LEFT",      UTF8_KEY_LEFT},
  {"RIGHT",     UTF8_KEY_RIGHT},
  
  // Navigation - mapped to UTF-8+ safe space (single byte)
  {"HOME",      UTF8_KEY_HOME},
  {"END",       UTF8_KEY_END},
  {"PAGEUP",    UTF8_KEY_PAGEUP},
  {"PAGEDOWN",  UTF8_KEY_PAGEDOWN},
  {"DELETE",    UTF8_KEY_DELETE},
  {"DEL",       UTF8_KEY_DELETE},      // Alias - will display as DELETE (first entry)
  
  // Common control keys - these map to printable characters (literal ASCII)
  {"ENTER",     '\n'},            // Newline character (0x0A)
  {"TAB",       '\t'},            // Tab character (0x09)
  {"SPACE",     ' '},             // Space character (0x20)
  {"ESC",       0x1B},            // Escape character
  {"BACKSPACE", 0x08},            // Backspace character
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

uint8_t findUTF8CodeForKeyword(const char* keyword) {
  // Handle function keys specially (they use 2-byte encoding)
  if (isFunctionKey(keyword)) {
    return UTF8_FUNCTION_KEY;  // Caller must handle 2-byte encoding
  }
  
  // Handle other keywords normally
  for (int i = 12; i < KEYWORD_TABLE_SIZE; i++) {  // Skip F1-F12 entries
    KeywordMapping entry;
    memcpy_P(&entry, &KEYWORD_TABLE[i], sizeof(KeywordMapping));
    if (strcasecmp(keyword, entry.keyword) == 0) {
      return entry.utf8Code;
    }
  }
  return 0;
}

uint8_t findModifierBit(const char* name) {
  if (strcasecmp(name, "CTRL") == 0) return MULTI_CTRL;
  if (strcasecmp(name, "SHIFT") == 0) return MULTI_SHIFT;
  if (strcasecmp(name, "ALT") == 0) return MULTI_ALT;
  if (strcasecmp(name, "WIN") == 0 || strcasecmp(name, "GUI") == 0 || strcasecmp(name, "CMD") == 0) return MULTI_CMD;
  return 0;
}

const char* findKeywordForUTF8Code(uint8_t utf8Code) {
  // Handle function key lookup (2-byte encoding not supported here)
  // This function is used by decoder which handles 2-byte sequences separately
  
  for (int i = 12; i < KEYWORD_TABLE_SIZE; i++) {  // Skip F1-F12 entries
    KeywordMapping entry;
    memcpy_P(&entry, &KEYWORD_TABLE[i], sizeof(KeywordMapping));
    if (entry.utf8Code == utf8Code) {
      return entry.keyword;  // First match is preferred display name
    }
  }
  return nullptr; // Not found
}

// Function key specific helpers
bool isFunctionKey(const char* keyword) {
  if (strlen(keyword) < 2 || keyword[0] != 'F') return false;
  
  // Check F1-F12
  if (strcmp(keyword, "F1") == 0 || strcmp(keyword, "F2") == 0 || strcmp(keyword, "F3") == 0 ||
      strcmp(keyword, "F4") == 0 || strcmp(keyword, "F5") == 0 || strcmp(keyword, "F6") == 0 ||
      strcmp(keyword, "F7") == 0 || strcmp(keyword, "F8") == 0 || strcmp(keyword, "F9") == 0 ||
      strcmp(keyword, "F10") == 0 || strcmp(keyword, "F11") == 0 || strcmp(keyword, "F12") == 0) {
    return true;
  }
  return false;
}

uint8_t getFunctionKeyNumber(const char* keyword) {
  if (strcmp(keyword, "F1") == 0) return 1;
  if (strcmp(keyword, "F2") == 0) return 2;
  if (strcmp(keyword, "F3") == 0) return 3;
  if (strcmp(keyword, "F4") == 0) return 4;
  if (strcmp(keyword, "F5") == 0) return 5;
  if (strcmp(keyword, "F6") == 0) return 6;
  if (strcmp(keyword, "F7") == 0) return 7;
  if (strcmp(keyword, "F8") == 0) return 8;
  if (strcmp(keyword, "F9") == 0) return 9;
  if (strcmp(keyword, "F10") == 0) return 10;
  if (strcmp(keyword, "F11") == 0) return 11;
  if (strcmp(keyword, "F12") == 0) return 12;
  return 0; // Not a function key
}

//==============================================================================
// SHARED UTILITY FUNCTIONS
//==============================================================================

bool isRegularCharacter(uint8_t b) {
  // Regular printable ASCII characters, control characters that are literal,
  // and UTF-8 extended characters
  // Exclude our UTF-8+ control codes and special key codes
  
  return !isUTF8ControlCode(b);
}

bool needsQuoting(uint8_t b) {
  // Characters that should be quoted even if single
  return (b == ' ' || b == '\t' || b == '\n' || b == '\r' || 
          b == '"' || b == '\\' || b < 0x20);
}

bool isUTF8ControlCode(uint8_t b) {
  // Check for all UTF-8+ control and special key codes
  //
  if (b == 0x1B) return false;  // ESCAPE
  
  // Modifier control codes
  if (b >= UTF8_PRESS_CTRL && b <= UTF8_PRESS_CMD) return true;  // 0x01-0x04
  if (b == UTF8_FUNCTION_KEY) return true;  // 0x05 (function key prefix)
  if (b == UTF8_RELEASE_CTRL) return true;  // 0x06
  if (b == UTF8_KEY_UP) return true;  // 0x07
  if (b == UTF8_PRESS_MULTI || b == UTF8_RELEASE_MULTI) return true;  // 0x0E-0x0F
  if (b >= UTF8_RELEASE_ALT && b <= UTF8_RELEASE_CMD) return true;  // 0x10-0x12
  if (b >= UTF8_KEY_DOWN && b <= UTF8_KEY_DELETE) return true;  // 0x13-0x1A
  
  return false;
}
