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
// SHARED DATA STRUCTURES
//==============================================================================

struct KeywordMapping {
  const char* keyword;
  uint8_t hidCode;
};

struct ModifierInfo {
  const char* name;
  uint8_t multiBit;
};

struct ParsedMapCommand {
  bool valid;
  uint8_t keyIndex;
  String event;          // "down", "up", or "" (default down)
  String utf8Sequence;   // Generated UTF-8+ encoded macro
  String errorMessage;
  
  ParsedMapCommand() : valid(false), keyIndex(0) {}
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

// Find HID code for keyword (used by parser)
uint8_t findHIDCodeForKeyword(const String& keyword);

// Find keyword for HID code (used by decompiler)
// Returns first matching entry, which is the preferred display name
const char* findKeywordForHID(uint8_t hidCode);

// Find modifier bit mask for name
uint8_t findModifierBit(const String& name);

//==============================================================================
// SHARED UTILITY FUNCTIONS
//==============================================================================

// Character classification helpers
bool isRegularCharacter(uint8_t b);
bool needsQuoting(uint8_t b);

#endif // MAP_PARSER_TABLES_H