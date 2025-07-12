/*
 * Macro Execution Engine Implementation
 * 
 * Executes UTF-8+ encoded macro sequences directly via USB HID
 */

#include "map-parser-tables.h"
#include <Keyboard.h>

//==============================================================================
// FUNCTION KEY HID CODE LOOKUP TABLE
//==============================================================================

// Map function key numbers (1-12) to HID key codes
// Index 0 is unused, indices 1-12 correspond to F1-F12
static const uint16_t FUNCTION_KEY_CODES[] = {
  0x00,        // Index 0 - unused
  KEY_F1,      // Index 1 - F1 = 0x3A
  KEY_F2,      // Index 2 - F2 = 0x3B
  KEY_F3,      // Index 3 - F3 = 0x3C
  KEY_F4,      // Index 4 - F4 = 0x3D
  KEY_F5,      // Index 5 - F5 = 0x3E
  KEY_F6,      // Index 6 - F6 = 0x3F
  KEY_F7,      // Index 7 - F7 = 0x40
  KEY_F8,      // Index 8 - F8 = 0x41
  KEY_F9,      // Index 9 - F9 = 0x42
  KEY_F10,     // Index 10 - F10 = 0x43
  KEY_F11,     // Index 11 - F11 = 0x44
  KEY_F12      // Index 12 - F12 = 0x45
};

#define NUM_FUNCTION_KEYS 12

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
      
      // Function key 2-byte encoding
      case UTF8_FUNCTION_KEY:
        if (i + 1 < length) {
          uint8_t keyNum = bytes[++i];
          // Validate function key number (1-12)
          if (keyNum >= 1 && keyNum <= NUM_FUNCTION_KEYS) {
            uint16_t hidCode = FUNCTION_KEY_CODES[keyNum];
            Keyboard.press(hidCode);

            Keyboard.release(hidCode);
          }
          // If invalid, silently ignore (as requested)
        }
        // If no next byte available, silently ignore
        break;
      
      // All other bytes are direct HID codes or printable characters
      default:
        Keyboard.write(b);
    }
  }
}

void initializeMacroEngine() {
}
