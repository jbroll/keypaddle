/*
 * MAP Parser Shared Tables and Definitions
 * 
 * Shared lookup tables, constants, and structures used by:
 * - MAP command parser
 * - Macro execution engine  
 * - Macro decompiler
 */

#ifndef MAP_PARSER_TABLES_H
#define MAP_PARSER_TABLES_H

#include <Arduino.h>

//==============================================================================
// UTF-8+ CONTROL CODES - PRESS/RELEASE PRIMITIVES ONLY
//==============================================================================

// Individual modifier press/release - UPDATED to avoid conflicts
#define UTF8_PRESS_CTRL      0x01  // Press and hold Ctrl (SOH - Start of Heading)
#define UTF8_PRESS_ALT       0x02  // Press and hold Alt (STX - Start of Text)
#define UTF8_PRESS_SHIFT     0x03  // Press and hold Shift (ETX - End of Text)
#define UTF8_PRESS_CMD       0x04  // Press and hold CMD/GUI (EOT - End of Transmission)
#define UTF8_RELEASE_CTRL    0x06  // Release Ctrl (ACK - Acknowledge)
#define UTF8_RELEASE_ALT     0x10  // Release Alt (DLE - Data Link Escape)
#define UTF8_RELEASE_SHIFT   0x11  // Release Shift (DC1 - Device Control 1)
#define UTF8_RELEASE_CMD     0x12  // Release CMD/GUI (DC2 - Device Control 2)

// Multi-modifier operations (2-byte: opcode + mask) - these were already safe
#define UTF8_PRESS_MULTI     0x0E  // Press modifiers in next byte (SO - Shift Out)
#define UTF8_RELEASE_MULTI   0x0F  // Release modifiers in next byte (SI - Shift In)

// Multi-modifier bit masks (unchanged)
#define MULTI_CTRL           0x01
#define MULTI_SHIFT          0x02  
#define MULTI_ALT            0x04
#define MULTI_CMD            0x08

//==============================================================================
// UTF-8+ SPECIAL KEY CODES - SAFE NAMESPACE
//==============================================================================

// Function keys: 2-byte encoding [0x05, key_number]
#define UTF8_FUNCTION_KEY    0x05  // Followed by function key number (1-12)
// Usage: [0x05, 0x01] = F1, [0x05, 0x02] = F2, ..., [0x05, 0x0C] = F12

// Navigation and special keys: Single-byte using available safe codes
#define UTF8_KEY_UP          0x13
#define UTF8_KEY_DOWN        0x14
#define UTF8_KEY_LEFT        0x15
#define UTF8_KEY_RIGHT       0x16
#define UTF8_KEY_HOME        0x17
#define UTF8_KEY_END         0x18
#define UTF8_KEY_PAGEUP      0x19
#define UTF8_KEY_PAGEDOWN    0x1A
// 0x01B is ESC do not use that.
#define UTF8_KEY_DELETE      0x1C
                                 
// Reserved for future expansion: 0x1D, 0x1Em 0x1F

// Special character keys that were previously keywords
// These now map to their literal ASCII values in the encoder/decoder
// ENTER -> '\n' (0x0A), TAB -> '\t' (0x09), etc.

//==============================================================================
// SHARED DATA STRUCTURES
//==============================================================================

struct KeywordMapping {
  const char* keyword;
  uint8_t utf8Code;  // Changed from hidCode to utf8Code
};

struct ModifierInfo {
  const char* name;
  uint8_t multiBit;
};

//==============================================================================
// EXTERNAL TABLE DECLARATIONS
//==============================================================================

extern const KeywordMapping KEYWORD_TABLE[] PROGMEM;
extern const int KEYWORD_TABLE_SIZE;

extern const ModifierInfo MODIFIERS[] PROGMEM;
extern const int NUM_MODIFIERS;

//==============================================================================
// SHARED LOOKUP FUNCTIONS
//==============================================================================

uint8_t findUTF8CodeForKeyword(const char* keyword);
uint8_t findModifierBit(const char* name);
const char* findKeywordForUTF8Code(uint8_t utf8Code);

// Function key specific helpers
bool isFunctionKey(const char* keyword);  // Returns true for F1-F12
uint8_t getFunctionKeyNumber(const char* keyword);  // Returns 1-12 for F1-F12, 0 if not a function key

//==============================================================================
// SHARED UTILITY FUNCTIONS
//==============================================================================

// Character classification helpers
bool isRegularCharacter(uint8_t b);
bool needsQuoting(uint8_t b);
bool isUTF8ControlCode(uint8_t b);  // Helper to identify all UTF-8+ control codes

#endif // MAP_PARSER_TABLES_H
