/*
 * Simplified UTF-8+ Macro Engine - Direct HID Codes
 * 
 * Eliminates SPECIAL key system - stores HID codes directly in macro bytes
 * Table-driven compilation and decompilation for maintainability
 */

#ifndef MACRO_ENGINE_H
#define MACRO_ENGINE_H

#include <Arduino.h>
#include <Keyboard.h>

//==============================================================================
// UTF-8+ CONTROL CODES - PRESS/RELEASE PRIMITIVES ONLY
//==============================================================================

// Individual modifier press/release
#define UTF8_PRESS_CTRL      0x1A  // Press and hold Ctrl
#define UTF8_PRESS_ALT       0x1B  // Press and hold Alt  
#define UTF8_PRESS_SHIFT     0x1C  // Press and hold Shift
#define UTF8_PRESS_CMD       0x1D  // Press and hold CMD (GUI)
#define UTF8_RELEASE_CTRL    0x1E  // Release Ctrl
#define UTF8_RELEASE_ALT     0x1F  // Release Alt
#define UTF8_RELEASE_SHIFT   0x05  // Release Shift
#define UTF8_RELEASE_CMD     0x19  // Release CMD (GUI)

// Multi-modifier operations (2-byte: opcode + mask)
#define UTF8_PRESS_MULTI     0x0E  // Press modifiers in next byte
#define UTF8_RELEASE_MULTI   0x0F  // Release modifiers in next byte

// Multi-modifier bit masks
#define MULTI_CTRL           0x01
#define MULTI_SHIFT          0x02  
#define MULTI_ALT            0x04
#define MULTI_CMD            0x08

// Special control codes that need names (not direct HID codes)
#define UTF8_TAB             0x09  // Tab key (same as HID, but named for clarity)
#define UTF8_ENTER           0x0A  // Enter key (same as HID, but named for clarity)
#define UTF8_ESCAPE          0x0B  // Escape key
#define UTF8_BACKSPACE       0x08  // Backspace key

//==============================================================================
// KEYWORD TO HID CODE MAPPING TABLE
//==============================================================================

struct KeywordMapping {
  const char* keyword;
  uint8_t hidCode;
};

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
// TABLE-DRIVEN LOOKUP FUNCTIONS
//==============================================================================

// Find HID code for keyword (used by parser)
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

// Find keyword for HID code (used by decompiler)
// Returns first matching entry, which is the preferred display name
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
// SIMPLIFIED EXECUTION ENGINE
//==============================================================================

void executeUTF8Macro(const uint8_t* bytes, uint16_t length) {
  if (!bytes || length == 0) return;
  
  for (uint16_t i = 0; i < length; i++) {
    uint8_t b = bytes[i];
    
    switch (b) {
      // Individual modifier press operations
      case UTF8_PRESS_CTRL:    Keyboard.press(KEY_LEFT_CTRL); break;
      case UTF8_PRESS_ALT:     Keyboard.press(KEY_LEFT_ALT); break;
      case UTF8_PRESS_SHIFT:   Keyboard.press(KEY_LEFT_SHIFT); break;
      case UTF8_PRESS_CMD:     Keyboard.press(KEY_LEFT_GUI); break;
      
      // Individual modifier release operations
      case UTF8_RELEASE_CTRL:  Keyboard.release(KEY_LEFT_CTRL); break;
      case UTF8_RELEASE_ALT:   Keyboard.release(KEY_LEFT_ALT); break;
      case UTF8_RELEASE_SHIFT: Keyboard.release(KEY_LEFT_SHIFT); break;
      case UTF8_RELEASE_CMD:   Keyboard.release(KEY_LEFT_GUI); break;
      
      // Multi-modifier operations
      case UTF8_PRESS_MULTI:
        if (i + 1 < length) {
          uint8_t mask = bytes[++i];
          if (mask & MULTI_CTRL)  Keyboard.press(KEY_LEFT_CTRL);
          if (mask & MULTI_SHIFT) Keyboard.press(KEY_LEFT_SHIFT);
          if (mask & MULTI_ALT)   Keyboard.press(KEY_LEFT_ALT);
          if (mask & MULTI_CMD)   Keyboard.press(KEY_LEFT_GUI);
        }
        break;
        
      case UTF8_RELEASE_MULTI:
        if (i + 1 < length) {
          uint8_t mask = bytes[++i];
          if (mask & MULTI_CTRL)  Keyboard.release(KEY_LEFT_CTRL);
          if (mask & MULTI_SHIFT) Keyboard.release(KEY_LEFT_SHIFT);
          if (mask & MULTI_ALT)   Keyboard.release(KEY_LEFT_ALT);
          if (mask & MULTI_CMD)   Keyboard.release(KEY_LEFT_GUI);
        }
        break;
      
      // All other bytes are direct HID codes or printable characters
      default:
        if (b >= 0x20 && b <= 0x7E) {
          // Printable ASCII - send directly
          Keyboard.write(b);
        } else if (b > 0x7E) {
          // UTF-8 extended characters - send directly
          Keyboard.write(b);
        } else if (b >= 0x04) {
          // HID key codes (0x04-0x1F range that aren't our control codes)
          Keyboard.write(b);
        }
        // Ignore other control characters (0x00-0x03, etc.)
        break;
    }
  }
}

//==============================================================================
// INTELLIGENT DECOMPILER WITH STRING RECOGNITION
//==============================================================================

String decodeUTF8Macro(const uint8_t* bytes, uint16_t length) {
  if (!bytes || length == 0) return F("(empty)");
  
  String result = "";
  
  for (uint16_t i = 0; i < length; ) {
    uint8_t b = bytes[i];
    
    switch (b) {
      // Handle control codes
      case UTF8_PRESS_CTRL:    result += F("[+CTRL]"); i++; break;
      case UTF8_PRESS_ALT:     result += F("[+ALT]"); i++; break;
      case UTF8_PRESS_SHIFT:   result += F("[+SHIFT]"); i++; break;
      case UTF8_PRESS_CMD:     result += F("[+WIN]"); i++; break;
      case UTF8_RELEASE_CTRL:  result += F("[-CTRL]"); i++; break;
      case UTF8_RELEASE_ALT:   result += F("[-ALT]"); i++; break;
      case UTF8_RELEASE_SHIFT: result += F("[-SHIFT]"); i++; break;
      case UTF8_RELEASE_CMD:   result += F("[-WIN]"); i++; break;
      
      case UTF8_PRESS_MULTI:
        if (i + 1 < length) {
          uint8_t mask = bytes[i + 1];
          result += F("[+");
          bool needPlus = false;
          if (mask & MULTI_CTRL)  { result += F("CTRL"); needPlus = true; }
          if (mask & MULTI_SHIFT) { if (needPlus) result += F("+"); result += F("SHIFT"); needPlus = true; }
          if (mask & MULTI_ALT)   { if (needPlus) result += F("+"); result += F("ALT"); needPlus = true; }
          if (mask & MULTI_CMD)   { if (needPlus) result += F("+"); result += F("WIN"); }
          result += F("]");
          i += 2;
        } else {
          i++;
        }
        break;
        
      case UTF8_RELEASE_MULTI:
        if (i + 1 < length) {
          uint8_t mask = bytes[i + 1];
          result += F("[-");
          bool needPlus = false;
          if (mask & MULTI_CTRL)  { result += F("CTRL"); needPlus = true; }
          if (mask & MULTI_SHIFT) { if (needPlus) result += F("+"); result += F("SHIFT"); needPlus = true; }
          if (mask & MULTI_ALT)   { if (needPlus) result += F("+"); result += F("ALT"); needPlus = true; }
          if (mask & MULTI_CMD)   { if (needPlus) result += F("+"); result += F("WIN"); }
          result += F("]");
          i += 2;
        } else {
          i++;
        }
        break;
      
      default:
        // Check if this is a regular character sequence
        if (isRegularCharacter(b)) {
          // Look ahead to find consecutive regular characters
          uint16_t stringStart = i;
          uint16_t stringLength = 0;
          
          while (i < length && isRegularCharacter(bytes[i])) {
            stringLength++;
            i++;
          }
          
          // If we found a sequence of regular characters, format as quoted string
          if (stringLength > 1 || needsQuoting(bytes[stringStart])) {
            result += F("\"");
            for (uint16_t j = stringStart; j < stringStart + stringLength; j++) {
              char c = (char)bytes[j];
              // Handle escape sequences
              if (c == '"') {
                result += F("\\\"");
              } else if (c == '\\') {
                result += F("\\\\");
              } else if (c == '\n') {
                result += F("\\n");
              } else if (c == '\r') {
                result += F("\\r");
              } else if (c == '\t') {
                result += F("\\t");
              } else {
                result += c;
              }
            }
            result += F("\"");
          } else {
            // Single character that doesn't need quoting
            result += (char)bytes[stringStart];
            i++;
          }
        } else {
          // Check if it's a known HID code with a keyword
          const char* keyword = findKeywordForHID(b);
          if (keyword) {
            result += F("[");
            result += keyword;
            result += F("]");
          } else {
            // Unknown byte - show as hex
            result += F("[0x");
            if (b < 0x10) result += F("0");
            result += String(b, HEX);
            result += F("]");
          }
          i++;
        }
        break;
    }
  }
  
  return result;
}

//==============================================================================
// HELPER FUNCTIONS FOR DECOMPILER
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

//==============================================================================
// PARSER HELPER FUNCTION
//==============================================================================

// Add a keyword to sequence (used by parser)
bool addKeywordToSequence(String& sequence, const String& keyword) {
  uint8_t hidCode = findHIDCodeForKeyword(keyword);
  if (hidCode != 0) {
    sequence += (char)hidCode;
    return true;
  }
  return false;
}

//==============================================================================
// INITIALIZATION
//==============================================================================

void initializeMacroEngine() {
  Keyboard.begin();
}

#endif // MACRO_ENGINE_H
