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
  
  // Common control keys - these map to printable characters
  {"ENTER",     '\n'},            // Newline character
  {"TAB",       '\t'},            // Tab character  
  {"SPACE",     ' '},             // Space character
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

uint8_t findHIDCodeForKeyword(const char* keyword) {
  for (int i = 0; i < KEYWORD_TABLE_SIZE; i++) {
    KeywordMapping entry;
    memcpy_P(&entry, &KEYWORD_TABLE[i], sizeof(KeywordMapping));
    if (strcasecmp(keyword, entry.keyword) == 0) {  // Pure C comparison
      return entry.hidCode;
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

//==============================================================================
// CHARACTER TO HID CODE MAPPING  
//==============================================================================

uint8_t charToHID(char c) {
  // Convert printable ASCII characters to HID codes
  if (c >= 'a' && c <= 'z') {
    return 0x04 + (c - 'a');  // HID codes 0x04-0x1D for a-z
  }
  if (c >= 'A' && c <= 'Z') {
    return 0x04 + (c - 'A');  // HID codes 0x04-0x1D for A-Z  
  }
  if (c >= '1' && c <= '9') {
    return 0x1E + (c - '1');  // HID codes 0x1E-0x26 for 1-9
  }
  if (c == '0') {
    return 0x27;              // HID code for 0
  }
  
  // Special characters
  switch (c) {
    case ' ':  return 0x2C;   // Space
    case '!':  return 0x1E;   // 1 with shift
    case '@':  return 0x1F;   // 2 with shift  
    case '#':  return 0x20;   // 3 with shift
    case '$':  return 0x21;   // 4 with shift
    case '%':  return 0x22;   // 5 with shift
    case '^':  return 0x23;   // 6 with shift
    case '&':  return 0x24;   // 7 with shift
    case '*':  return 0x25;   // 8 with shift
    case '(':  return 0x26;   // 9 with shift
    case ')':  return 0x27;   // 0 with shift
    case '-':  return 0x2D;   // Minus
    case '_':  return 0x2D;   // Minus with shift
    case '=':  return 0x2E;   // Equals
    case '+':  return 0x2E;   // Equals with shift
    case '[':  return 0x2F;   // Left bracket
    case '{':  return 0x2F;   // Left bracket with shift
    case ']':  return 0x30;   // Right bracket
    case '}':  return 0x30;   // Right bracket with shift
    case '\\': return 0x31;   // Backslash
    case '|':  return 0x31;   // Backslash with shift
    case ';':  return 0x33;   // Semicolon
    case ':':  return 0x33;   // Semicolon with shift
    case '\'': return 0x34;   // Quote
    case '"':  return 0x34;   // Quote with shift
    case '`':  return 0x35;   // Backtick
    case '~':  return 0x35;   // Backtick with shift
    case ',':  return 0x36;   // Comma
    case '<':  return 0x36;   // Comma with shift
    case '.':  return 0x37;   // Period
    case '>':  return 0x37;   // Period with shift
    case '/':  return 0x38;   // Slash
    case '?':  return 0x38;   // Slash with shift
    default:   return c;      // Pass through other characters
  }
}

char hidToChar(uint8_t hidCode) {
  // Convert HID codes back to characters
  if (hidCode >= 0x04 && hidCode <= 0x1D) {
    return 'a' + (hidCode - 0x04);  // a-z
  }
  if (hidCode >= 0x1E && hidCode <= 0x26) {
    return '1' + (hidCode - 0x1E);  // 1-9
  }
  if (hidCode == 0x27) {
    return '0';
  }
  
  // Special characters - return base character without shift
  switch (hidCode) {
    case 0x2C: return ' ';    // Space
    case 0x2D: return '-';    // Minus
    case 0x2E: return '=';    // Equals
    case 0x2F: return '[';    // Left bracket
    case 0x30: return ']';    // Right bracket
    case 0x31: return '\\';   // Backslash
    case 0x33: return ';';    // Semicolon
    case 0x34: return '\'';   // Quote
    case 0x35: return '`';    // Backtick
    case 0x36: return ',';    // Comma
    case 0x37: return '.';    // Period
    case 0x38: return '/';    // Slash
    default:   return hidCode; // Pass through
  }
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