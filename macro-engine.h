/*
 * UTF-8+ Macro Engine
 * 
 * Handles UTF-8+ encoding/decoding and macro execution
 * - Direct execution from encoded byte streams
 * - Unicode text support
 * - Efficient modifier key handling
 * - Human-readable debugging
 */

#ifndef MACRO_ENGINE_H
#define MACRO_ENGINE_H

#include <Arduino.h>
#include <Keyboard.h>

//==============================================================================
// UTF-8+ CONTROL CODES (C0 block 0x01-0x1F)
//==============================================================================

#define UTF8_CTRL_NEXT     0x01  // Press Ctrl with next character
#define UTF8_ALT_NEXT      0x02  // Press Alt with next character  
#define UTF8_GUI_NEXT      0x03  // Press Win/GUI with next character
#define UTF8_SHIFT_PRESS   0x04  // Begin shift mode
#define UTF8_SHIFT_RELEASE 0x05  // End shift mode
#define UTF8_BACKSPACE     0x08  // Backspace key
#define UTF8_TAB           0x09  // Tab key
#define UTF8_ENTER         0x0A  // Enter key
#define UTF8_ESCAPE        0x0B  // Escape key
#define UTF8_MULTI_MOD     0x0E  // Multi-modifier combination
#define UTF8_SPECIAL_KEY   0x0F  // Special key (F-keys, arrows, etc.)

//==============================================================================
// SPECIAL KEY CODES (for use with UTF8_SPECIAL_KEY)
//==============================================================================

#define SPECIAL_F1         0x01
#define SPECIAL_F2         0x02
#define SPECIAL_F3         0x03
#define SPECIAL_F4         0x04
#define SPECIAL_F5         0x05
#define SPECIAL_F6         0x06
#define SPECIAL_F7         0x07
#define SPECIAL_F8         0x08
#define SPECIAL_F9         0x09
#define SPECIAL_F10        0x0A
#define SPECIAL_F11        0x0B
#define SPECIAL_F12        0x0C
#define SPECIAL_UP         0x10
#define SPECIAL_DOWN       0x11
#define SPECIAL_LEFT       0x12
#define SPECIAL_RIGHT      0x13
#define SPECIAL_DELETE     0x14
#define SPECIAL_HOME       0x15
#define SPECIAL_END        0x16
#define SPECIAL_PAGE_UP    0x17
#define SPECIAL_PAGE_DOWN  0x18

//==============================================================================
// KEYBOARD KEY MAPPINGS
//==============================================================================

// Map special codes to Arduino Keyboard library constants
const uint8_t SPECIAL_KEY_MAP[] = {
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
  KEY_PAGE_DOWN        // 0x18
};

//==============================================================================
// MACRO EXECUTION ENGINE
//==============================================================================

// Execute UTF-8+ encoded macro
void executeUTF8Macro(const uint8_t* bytes, uint16_t length) {
  if (!bytes || length == 0) return;
  
  for (int i = 0; i < length; i++) {
    uint8_t b = bytes[i];
    
    // Control codes (0x01-0x1F)
    if (b >= 0x01 && b <= 0x1F) {
      switch (b) {
        case UTF8_CTRL_NEXT:
          if (i + 1 < length) {
            Keyboard.press(KEY_LEFT_CTRL);
            Keyboard.write(bytes[++i]);
            Keyboard.release(KEY_LEFT_CTRL);
          }
          break;
          
        case UTF8_ALT_NEXT:
          if (i + 1 < length) {
            Keyboard.press(KEY_LEFT_ALT);
            Keyboard.write(bytes[++i]);
            Keyboard.release(KEY_LEFT_ALT);
          }
          break;
          
        case UTF8_GUI_NEXT:
          if (i + 1 < length) {
            Keyboard.press(KEY_LEFT_GUI);
            Keyboard.write(bytes[++i]);
            Keyboard.release(KEY_LEFT_GUI);
          }
          break;
          
        case UTF8_SHIFT_PRESS:
          Keyboard.press(KEY_LEFT_SHIFT);
          break;
          
        case UTF8_SHIFT_RELEASE:
          Keyboard.release(KEY_LEFT_SHIFT);
          break;
          
        case UTF8_BACKSPACE:
          Keyboard.write(KEY_BACKSPACE);
          break;
          
        case UTF8_TAB:
          Keyboard.write(KEY_TAB);
          break;
          
        case UTF8_ENTER:
          Keyboard.write(KEY_RETURN);
          break;
          
        case UTF8_ESCAPE:
          Keyboard.write(KEY_ESC);
          break;
          
        case UTF8_MULTI_MOD:
          if (i + 2 < length) {
            uint8_t modMask = bytes[++i];
            uint8_t key = bytes[++i];
            
            // Press modifiers based on mask
            if (modMask & 0x01) Keyboard.press(KEY_LEFT_CTRL);
            if (modMask & 0x02) Keyboard.press(KEY_LEFT_SHIFT);
            if (modMask & 0x04) Keyboard.press(KEY_LEFT_ALT);
            if (modMask & 0x08) Keyboard.press(KEY_LEFT_GUI);
            
            // Press key
            Keyboard.write(key);
            
            // Release modifiers
            if (modMask & 0x01) Keyboard.release(KEY_LEFT_CTRL);
            if (modMask & 0x02) Keyboard.release(KEY_LEFT_SHIFT);
            if (modMask & 0x04) Keyboard.release(KEY_LEFT_ALT);
            if (modMask & 0x08) Keyboard.release(KEY_LEFT_GUI);
          }
          break;
          
        case UTF8_SPECIAL_KEY:
          if (i + 1 < length) {
            uint8_t specialCode = bytes[++i];
            if (specialCode < sizeof(SPECIAL_KEY_MAP) && SPECIAL_KEY_MAP[specialCode] != 0) {
              Keyboard.write(SPECIAL_KEY_MAP[specialCode]);
            }
          }
          break;
          
        default:
          // Unknown control code - ignore
          break;
      }
    }
    // Regular UTF-8 characters (0x20-0xFF)
    else if (b >= 0x20) {
      Keyboard.write(b);
    }
    // 0x00 is null terminator - stop
    else if (b == 0x00) {
      break;
    }
  }
}

// Execute toggle modifier (press if not pressed, release if pressed)
bool modifierStates[4] = {false, false, false, false}; // CTRL, SHIFT, ALT, GUI

void executeToggleModifier(uint8_t modifierCode) {
  uint8_t keyCode;
  uint8_t stateIndex;
  
  switch (modifierCode) {
    case UTF8_CTRL_NEXT:
      keyCode = KEY_LEFT_CTRL;
      stateIndex = 0;
      break;
    case UTF8_SHIFT_PRESS:
      keyCode = KEY_LEFT_SHIFT;
      stateIndex = 1;
      break;
    case UTF8_ALT_NEXT:
      keyCode = KEY_LEFT_ALT;
      stateIndex = 2;
      break;
    case UTF8_GUI_NEXT:
      keyCode = KEY_LEFT_GUI;
      stateIndex = 3;
      break;
    default:
      return; // Unknown modifier
  }
  
  if (modifierStates[stateIndex]) {
    // Currently pressed - release it
    Keyboard.release(keyCode);
    modifierStates[stateIndex] = false;
  } else {
    // Currently released - press it
    Keyboard.press(keyCode);
    modifierStates[stateIndex] = true;
  }
}

//==============================================================================
// HUMAN-READABLE DISPLAY FUNCTIONS
//==============================================================================

// Convert UTF-8+ encoded macro to human-readable string
String decodeUTF8Macro(const uint8_t* bytes, uint16_t length) {
  if (!bytes || length == 0) return F("(empty)");
  
  String result = "";
  
  for (int i = 0; i < length; i++) {
    uint8_t b = bytes[i];
    
    if (b >= 0x01 && b <= 0x1F) {
      switch (b) {
        case UTF8_CTRL_NEXT:
          result += F("CTRL+");
          if (i + 1 < length) {
            result += (char)bytes[++i];
          }
          break;
        case UTF8_ALT_NEXT:
          result += F("ALT+");
          if (i + 1 < length) {
            result += (char)bytes[++i];
          }
          break;
        case UTF8_GUI_NEXT:
          result += F("WIN+");
          if (i + 1 < length) {
            result += (char)bytes[++i];
          }
          break;
        case UTF8_SHIFT_PRESS:
          result += F("<SHIFT>");
          break;
        case UTF8_SHIFT_RELEASE:
          result += F("</SHIFT>");
          break;
        case UTF8_BACKSPACE:
          result += F("[BACKSPACE]");
          break;
        case UTF8_TAB:
          result += F("[TAB]");
          break;
        case UTF8_ENTER:
          result += F("[ENTER]");
          break;
        case UTF8_ESCAPE:
          result += F("[ESC]");
          break;
        case UTF8_MULTI_MOD:
          if (i + 2 < length) {
            uint8_t modMask = bytes[++i];
            uint8_t key = bytes[++i];
            
            bool needPlus = false;
            if (modMask & 0x01) { result += F("CTRL"); needPlus = true; }
            if (modMask & 0x02) { if (needPlus) result += F("+"); result += F("SHIFT"); needPlus = true; }
            if (modMask & 0x04) { if (needPlus) result += F("+"); result += F("ALT"); needPlus = true; }
            if (modMask & 0x08) { if (needPlus) result += F("+"); result += F("WIN"); needPlus = true; }
            if (needPlus) result += F("+");
            result += (char)key;
          }
          break;
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
              default:
                result += F("[SPECIAL_");
                result += String(specialCode, HEX);
                result += F("]");
                break;
            }
          }
          break;
        default:
          result += F("[0x");
          if (b < 0x10) result += F("0");
          result += String(b, HEX);
          result += F("]");
          break;
      }
    } else if (b >= 0x20) {
      result += (char)b;
    } else if (b == 0x00) {
      break; // Null terminator
    }
  }
  
  return result;
}

//==============================================================================
// TESTING AND VALIDATION
//==============================================================================

void testUTF8Parsing() {
  Serial.println(F("\n=== UTF-8+ Macro Engine Test ==="));
  
  // Test cases: {description, encoded bytes, expected display}
  struct TestCase {
    const char* description;
    uint8_t bytes[16];
    uint8_t length;
    const char* expected;
  };
  
  TestCase tests[] = {
    {"Simple text", {'h','e','l','l','o'}, 5, "hello"},
    {"Ctrl+C", {UTF8_CTRL_NEXT, 'c'}, 2, "CTRL+c"},
    {"Alt+Tab", {UTF8_ALT_NEXT, UTF8_TAB}, 2, "ALT+[TAB]"},
    {"Win+L", {UTF8_GUI_NEXT, 'l'}, 2, "WIN+l"},
    {"F1 key", {UTF8_SPECIAL_KEY, SPECIAL_F1}, 2, "[F1]"},
    {"Up arrow", {UTF8_SPECIAL_KEY, SPECIAL_UP}, 2, "[UP]"},
    {"Multi-mod", {UTF8_MULTI_MOD, 0x05, 't'}, 3, "CTRL+ALT+t"}, // 0x05 = CTRL+ALT
    {"Shift block", {UTF8_SHIFT_PRESS, 'h', 'i', UTF8_SHIFT_RELEASE}, 4, "<SHIFT>hi</SHIFT>"}
  };
  
  for (int i = 0; i < 8; i++) {
    Serial.print(F("Test "));
    Serial.print(i + 1);
    Serial.print(F(": "));
    Serial.print(tests[i].description);
    Serial.print(F(" -> "));
    
    String decoded = decodeUTF8Macro(tests[i].bytes, tests[i].length);
    Serial.println(decoded);
    
    // Could add validation here if needed
  }
  
  Serial.println(F("=== UTF-8+ Test Complete ===\n"));
}

// Initialize the macro engine
void initializeMacroEngine() {
  // Reset modifier states
  for (int i = 0; i < 4; i++) {
    modifierStates[i] = false;
  }
  
  // Initialize Keyboard library
  Keyboard.begin();
  
  Serial.println(F("UTF-8+ Macro Engine initialized"));
}

// Release all held modifiers (emergency cleanup)
void releaseAllModifiers() {
  Keyboard.release(KEY_LEFT_CTRL);
  Keyboard.release(KEY_LEFT_SHIFT);
  Keyboard.release(KEY_LEFT_ALT);
  Keyboard.release(KEY_LEFT_GUI);
  
  for (int i = 0; i < 4; i++) {
    modifierStates[i] = false;
  }
  
  Serial.println(F("All modifiers released"));
}

// Get current modifier states
void printModifierStates() {
  Serial.print(F("Modifier states: "));
  Serial.print(F("CTRL="));
  Serial.print(modifierStates[0] ? F("ON") : F("OFF"));
  Serial.print(F(" SHIFT="));
  Serial.print(modifierStates[1] ? F("ON") : F("OFF"));
  Serial.print(F(" ALT="));
  Serial.print(modifierStates[2] ? F("ON") : F("OFF"));
  Serial.print(F(" WIN="));
  Serial.println(modifierStates[3] ? F("ON") : F("OFF"));
}

#endif // MACRO_ENGINE_H
