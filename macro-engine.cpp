/*
 * Macro Execution Engine Implementation
 * 
 * Executes UTF-8+ encoded macro sequences directly via USB HID
 */

#include "map-parser-tables.h"
#include <Keyboard.h>

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

void initializeMacroEngine() {
}
