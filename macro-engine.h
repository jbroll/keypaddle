/*
 * Simplified UTF-8+ Macro Engine
 * 
 * Press/release primitive-based execution with multi-modifier optimization
 * All complexity moved to parser - execution engine is simple and fast
 */

#ifndef MACRO_ENGINE_H
#define MACRO_ENGINE_H

#include <Arduino.h>
#include <Keyboard.h>

//==============================================================================
// UTF-8+ CONTROL CODES - PRESS/RELEASE PRIMITIVES
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

// Special keys
#define UTF8_TAB             0x09  // Tab key
#define UTF8_ENTER           0x0A  // Enter key
#define UTF8_ESCAPE          0x0B  // Escape key
#define UTF8_BACKSPACE       0x08  // Backspace key
#define UTF8_SPECIAL_KEY     0x0C  // Special key (F-keys, arrows) + code

// Special key codes (for use with UTF8_SPECIAL_KEY)
#define SPECIAL_F1           0x01
#define SPECIAL_F2           0x02
#define SPECIAL_F3           0x03
#define SPECIAL_F4           0x04
#define SPECIAL_F5           0x05
#define SPECIAL_F6           0x06
#define SPECIAL_F7           0x07
#define SPECIAL_F8           0x08
#define SPECIAL_F9           0x09
#define SPECIAL_F10          0x0A
#define SPECIAL_F11          0x0B
#define SPECIAL_F12          0x0C
#define SPECIAL_UP           0x10
#define SPECIAL_DOWN         0x11
#define SPECIAL_LEFT         0x12
#define SPECIAL_RIGHT        0x13
#define SPECIAL_DELETE       0x14
#define SPECIAL_HOME         0x15
#define SPECIAL_END          0x16
#define SPECIAL_PAGE_UP      0x17
#define SPECIAL_PAGE_DOWN    0x18
#define SPECIAL_SPACE        0x20

//==============================================================================
// SPECIAL KEY MAPPING TABLE
//==============================================================================

const uint8_t SPECIAL_KEY_MAP[] PROGMEM = {
  0,                    // 0x00 - unused
  KEY_F1,              // 0x01
  KEY_F2,              // 0x02
  KEY_F3,              // 0x03
  KEY_F4,              // 0x04
  KEY_F5,              // 0x05
  KEY_F6,              // 0x06
  KEY_F7,              // 0x07
  KEY_F8,              // 0x08
  KEY_F9,              // 0x09
  KEY_F10,             // 0x0A
  KEY_F11,             // 0x0B
  KEY_F12,             // 0x0C
  0, 0, 0,             // 0x0D-0x0F unused
  KEY_UP_ARROW,        // 0x10
  KEY_DOWN_ARROW,      // 0x11
  KEY_LEFT_ARROW,      // 0x12
  KEY_RIGHT_ARROW,     // 0x13
  KEY_DELETE,          // 0x14
  KEY_HOME,            // 0x15
  KEY_END,             // 0x16
  KEY_PAGE_UP,         // 0x17
  KEY_PAGE_DOWN,       // 0x18
  0, 0, 0, 0, 0, 0, 0, // 0x19-0x1F unused
  ' '                  // 0x20 - space character
};

//==============================================================================
// SIMPLIFIED EXECUTION ENGINE
//==============================================================================

void executeUTF8Macro(const uint8_t* bytes, uint16_t length) {
  if (!bytes || length == 0) return;
  
  for (uint16_t i = 0; i < length; i++) {
    uint8_t b = bytes[i];
    
    switch (b) {
      // Individual modifier press operations
      case UTF8_PRESS_CTRL:
        Keyboard.press(KEY_LEFT_CTRL);
        break;
        
      case UTF8_PRESS_ALT:
        Keyboard.press(KEY_LEFT_ALT);
        break;
        
      case UTF8_PRESS_SHIFT:
        Keyboard.press(KEY_LEFT_SHIFT);
        break;
        
      case UTF8_PRESS_CMD:
        Keyboard.press(KEY_LEFT_GUI);
        break;
        
      // Individual modifier release operations
      case UTF8_RELEASE_CTRL:
        Keyboard.release(KEY_LEFT_CTRL);
        break;
        
      case UTF8_RELEASE_ALT:
        Keyboard.release(KEY_LEFT_ALT);
        break;
        
      case UTF8_RELEASE_SHIFT:
        Keyboard.release(KEY_LEFT_SHIFT);
        break;
        
      case UTF8_RELEASE_CMD:
        Keyboard.release(KEY_LEFT_GUI);
        break;
        
      // Multi-modifier press operation
      case UTF8_PRESS_MULTI:
        if (i + 1 < length) {
          uint8_t mask = bytes[++i];
          if (mask & MULTI_CTRL)  Keyboard.press(KEY_LEFT_CTRL);
          if (mask & MULTI_SHIFT) Keyboard.press(KEY_LEFT_SHIFT);
          if (mask & MULTI_ALT)   Keyboard.press(KEY_LEFT_ALT);
          if (mask & MULTI_CMD)   Keyboard.press(KEY_LEFT_GUI);
        }
        break;
        
      // Multi-modifier release operation
      case UTF8_RELEASE_MULTI:
        if (i + 1 < length) {
          uint8_t mask = bytes[++i];
          if (mask & MULTI_CTRL)  Keyboard.release(KEY_LEFT_CTRL);
          if (mask & MULTI_SHIFT) Keyboard.release(KEY_LEFT_SHIFT);
          if (mask & MULTI_ALT)   Keyboard.release(KEY_LEFT_ALT);
          if (mask & MULTI_CMD)   Keyboard.release(KEY_LEFT_GUI);
        }
        break;
        
      // Direct special keys
      case UTF8_TAB:
        Keyboard.write(KEY_TAB);
        break;
        
      case UTF8_ENTER:
        Keyboard.write(KEY_RETURN);
        break;
        
      case UTF8_ESCAPE:
        Keyboard.write(KEY_ESC);
        break;
        
      case UTF8_BACKSPACE:
        Keyboard.write(KEY_BACKSPACE);
        break;
        
      // Special key with code
      case UTF8_SPECIAL_KEY:
        if (i + 1 < length) {
          uint8_t specialCode = bytes[++i];
          if (specialCode < sizeof(SPECIAL_KEY_MAP)) {
            uint8_t keyCode = pgm_read_byte(&SPECIAL_KEY_MAP[specialCode]);
            if (keyCode != 0) {
              if (keyCode == ' ') {
                Keyboard.write(' ');
              } else {
                Keyboard.write(keyCode);
              }
            }
          }
        }
        break;
        
      // Regular UTF-8 characters
      default:
        if (b >= 0x20 && b <= 0xFF) {
          Keyboard.write(b);
        }
        // Ignore control characters 0x00-0x1F not handled above
        break;
    }
  }
}

//==============================================================================
// HUMAN-READABLE DISPLAY DECODER
//==============================================================================

String decodeUTF8Macro(const uint8_t* bytes, uint16_t length) {
  if (!bytes || length == 0) return F("(empty)");
  
  String result = "";
  
  for (uint16_t i = 0; i < length; i++) {
    uint8_t b = bytes[i];
    
    switch (b) {
      case UTF8_PRESS_CTRL:    result += F("[+CTRL]"); break;
      case UTF8_PRESS_ALT:     result += F("[+ALT]"); break;
      case UTF8_PRESS_SHIFT:   result += F("[+SHIFT]"); break;
      case UTF8_PRESS_CMD:     result += F("[+WIN]"); break;
      case UTF8_RELEASE_CTRL:  result += F("[-CTRL]"); break;
      case UTF8_RELEASE_ALT:   result += F("[-ALT]"); break;
      case UTF8_RELEASE_SHIFT: result += F("[-SHIFT]"); break;
      case UTF8_RELEASE_CMD:   result += F("[-WIN]"); break;
      
      case UTF8_PRESS_MULTI:
        if (i + 1 < length) {
          uint8_t mask = bytes[++i];
          result += F("[+");
          bool needPlus = false;
          if (mask & MULTI_CTRL)  { result += F("CTRL"); needPlus = true; }
          if (mask & MULTI_SHIFT) { if (needPlus) result += F("+"); result += F("SHIFT"); needPlus = true; }
          if (mask & MULTI_ALT)   { if (needPlus) result += F("+"); result += F("ALT"); needPlus = true; }
          if (mask & MULTI_CMD)   { if (needPlus) result += F("+"); result += F("WIN"); }
          result += F("]");
        }
        break;
        
      case UTF8_RELEASE_MULTI:
        if (i + 1 < length) {
          uint8_t mask = bytes[++i];
          result += F("[-");
          bool needPlus = false;
          if (mask & MULTI_CTRL)  { result += F("CTRL"); needPlus = true; }
          if (mask & MULTI_SHIFT) { if (needPlus) result += F("+"); result += F("SHIFT"); needPlus = true; }
          if (mask & MULTI_ALT)   { if (needPlus) result += F("+"); result += F("ALT"); needPlus = true; }
          if (mask & MULTI_CMD)   { if (needPlus) result += F("+"); result += F("WIN"); }
          result += F("]");
        }
        break;
        
      case UTF8_TAB:       result += F("[TAB]"); break;
      case UTF8_ENTER:     result += F("[ENTER]"); break;
      case UTF8_ESCAPE:    result += F("[ESC]"); break;
      case UTF8_BACKSPACE: result += F("[BACKSPACE]"); break;
      
      case UTF8_SPECIAL_KEY:
        if (i + 1 < length) {
          uint8_t specialCode = bytes[++i];
          switch (specialCode) {
            case SPECIAL_F1:  result += F("[F1]"); break;
            case SPECIAL_F2:  result += F("[F2]"); break;
            case SPECIAL_F3:  result += F("[F3]"); break;
            case SPECIAL_F4:  result += F("[F4]"); break;
            case SPECIAL_F5:  result += F("[F5]"); break;
            case SPECIAL_F6:  result += F("[F6]"); break;
            case SPECIAL_F7:  result += F("[F7]"); break;
            case SPECIAL_F8:  result += F("[F8]"); break;
            case SPECIAL_F9:  result += F("[F9]"); break;
            case SPECIAL_F10: result += F("[F10]"); break;
            case SPECIAL_F11: result += F("[F11]"); break;
            case SPECIAL_F12: result += F("[F12]"); break;
            case SPECIAL_UP:  result += F("[UP]"); break;
            case SPECIAL_DOWN: result += F("[DOWN]"); break;
            case SPECIAL_LEFT: result += F("[LEFT]"); break;
            case SPECIAL_RIGHT: result += F("[RIGHT]"); break;
            case SPECIAL_DELETE: result += F("[DELETE]"); break;
            case SPECIAL_HOME: result += F("[HOME]"); break;
            case SPECIAL_END: result += F("[END]"); break;
            case SPECIAL_PAGE_UP: result += F("[PAGE_UP]"); break;
            case SPECIAL_PAGE_DOWN: result += F("[PAGE_DOWN]"); break;
            case SPECIAL_SPACE: result += F("[SPACE]"); break;
            default:
              result += F("[SPECIAL_0x");
              if (specialCode < 0x10) result += F("0");
              result += String(specialCode, HEX);
              result += F("]");
              break;
          }
        }
        break;
        
      default:
        if (b >= 0x20 && b <= 0x7E) {
          result += (char)b;
        } else if (b > 0x7E) {
          // UTF-8 continuation or extended character
          result += (char)b;
        }
        // Ignore other control characters
        break;
    }
  }
  
  return result;
}

//==============================================================================
// INITIALIZATION AND CLEANUP
//==============================================================================

void initializeMacroEngine() {
  Keyboard.begin();
}

#endif // MACRO_ENGINE_H
