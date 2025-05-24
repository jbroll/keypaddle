/*
 * Single-Pass MAP Command Parser
 * 
 * Directly converts MAP command arguments to UTF-8+ byte sequences
 * No intermediate representations - parse and generate in one pass
 */

#ifndef MAP_PARSER_H
#define MAP_PARSER_H

#include <Arduino.h>

//==============================================================================
// PARSER RESULT STRUCTURE
//==============================================================================

struct ParsedMapCommand {
  bool valid;
  uint8_t keyIndex;
  String event;          // "down", "up", or "" (default down)
  String utf8Sequence;   // Generated UTF-8+ encoded macro
  String errorMessage;
  
  ParsedMapCommand() : valid(false), keyIndex(0) {}
};

//==============================================================================
// LOOKUP TABLES
//==============================================================================

struct ModifierInfo {
  const char* name;
  uint8_t multiBit;
};

const ModifierInfo MODIFIERS[] PROGMEM = {
  {"CTRL",  MULTI_CTRL},
  {"ALT",   MULTI_ALT}, 
  {"SHIFT", MULTI_SHIFT},
  {"CMD",   MULTI_GUI},
};
const int NUM_MODIFIERS = 4;

struct SpecialKeyInfo {
  const char* name;
  uint8_t directCode;
  uint8_t specialCode;
};

const SpecialKeyInfo SPECIAL_KEYS[] PROGMEM = {
  {"F1",       0, SPECIAL_F1},     {"F2",       0, SPECIAL_F2},
  {"F3",       0, SPECIAL_F3},     {"F4",       0, SPECIAL_F4},
  {"F5",       0, SPECIAL_F5},     {"F6",       0, SPECIAL_F6},
  {"F7",       0, SPECIAL_F7},     {"F8",       0, SPECIAL_F8},
  {"F9",       0, SPECIAL_F9},     {"F10",      0, SPECIAL_F10},
  {"F11",      0, SPECIAL_F11},    {"F12",      0, SPECIAL_F12},
  {"UP",       0, SPECIAL_UP},     {"DOWN",     0, SPECIAL_DOWN},
  {"LEFT",     0, SPECIAL_LEFT},   {"RIGHT",    0, SPECIAL_RIGHT},
  {"DELETE",   0, SPECIAL_DELETE}, {"DEL",      0, SPECIAL_DELETE},
  {"HOME",     0, SPECIAL_HOME},   {"END",      0, SPECIAL_END},
  {"PAGEUP",   0, SPECIAL_PAGE_UP}, {"PAGEDOWN", 0, SPECIAL_PAGE_DOWN},
  {"ENTER",    UTF8_ENTER, 0},     {"TAB",      UTF8_TAB, 0},
  {"ESC",      UTF8_ESCAPE, 0},    {"BACKSPACE", UTF8_BACKSPACE, 0},
  {"SPACE",    0, SPECIAL_SPACE}
};
const int NUM_SPECIAL_KEYS = 25;

//==============================================================================
// UTILITY FUNCTIONS
//==============================================================================

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

bool findSpecialKey(const String& name, uint8_t& directCode, uint8_t& specialCode) {
  for (int i = 0; i < NUM_SPECIAL_KEYS; i++) {
    SpecialKeyInfo key;
    memcpy_P(&key, &SPECIAL_KEYS[i], sizeof(SpecialKeyInfo));
    if (name.equalsIgnoreCase(key.name)) {
      directCode = key.directCode;
      specialCode = key.specialCode;
      return true;
    }
  }
  return false;
}

void addModifierPress(String& sequence, uint8_t modifierMask, bool useMulti) {
  if (modifierMask == 0) return;
  
  if (useMulti) {
    sequence += (char)UTF8_PRESS_MULTI;
    sequence += (char)modifierMask;
  } else {
    if (modifierMask & MULTI_CTRL)  sequence += (char)UTF8_PRESS_CTRL;
    if (modifierMask & MULTI_SHIFT) sequence += (char)UTF8_PRESS_SHIFT;
    if (modifierMask & MULTI_ALT)   sequence += (char)UTF8_PRESS_ALT;
    if (modifierMask & MULTI_GUI)   sequence += (char)UTF8_PRESS_GUI;
  }
}

void addModifierRelease(String& sequence, uint8_t modifierMask, bool useMulti) {
  if (modifierMask == 0) return;
  
  if (useMulti) {
    sequence += (char)UTF8_RELEASE_MULTI;
    sequence += (char)modifierMask;
  } else {
    if (modifierMask & MULTI_CTRL)  sequence += (char)UTF8_RELEASE_CTRL;
    if (modifierMask & MULTI_SHIFT) sequence += (char)UTF8_RELEASE_SHIFT;
    if (modifierMask & MULTI_ALT)   sequence += (char)UTF8_RELEASE_ALT;
    if (modifierMask & MULTI_GUI)   sequence += (char)UTF8_RELEASE_GUI;
  }
}

void addSpecialKey(String& sequence, const String& keyName) {
  uint8_t directCode, specialCode;
  if (findSpecialKey(keyName, directCode, specialCode)) {
    if (directCode != 0) {
      sequence += (char)directCode;
    } else {
      sequence += (char)UTF8_SPECIAL_KEY;
      sequence += (char)specialCode;
    }
  }
}

void processEscapeSequences(String& sequence, const String& text) {
  for (int i = 0; i < text.length(); i++) {
    if (text[i] == '\\' && i + 1 < text.length()) {
      char next = text[i + 1];
      switch (next) {
        case 'n':  sequence += (char)UTF8_ENTER; i++; break;
        case 'r':  sequence += '\r'; i++; break;
        case 't':  sequence += (char)UTF8_TAB; i++; break;
        case 'a':  i++; break; // Skip bell
        case '"':  sequence += '"';  i++; break;
        case '\\': sequence += '\\'; i++; break;
        default:   sequence += text[i]; break;
      }
    } else {
      sequence += text[i];
    }
  }
}

//==============================================================================
// SINGLE-PASS PARSER
//==============================================================================

ParsedMapCommand parseMapCommand(const String& input) {
  ParsedMapCommand result;
  String sequence = "";
  
  String trimmed = input;
  trimmed.trim();
  
  // Must start with MAP
  String upperInput = trimmed;
  upperInput.toUpperCase();
  if (!upperInput.startsWith("MAP ")) {
    result.errorMessage = "Command must start with MAP";
    return result;
  }
  
  // Parse character by character
  int pos = 4; // Skip "MAP "
  
  // Skip whitespace
  while (pos < trimmed.length() && isspace(trimmed[pos])) pos++;
  
  // Parse key index
  String keyStr = "";
  while (pos < trimmed.length() && isdigit(trimmed[pos])) {
    keyStr += trimmed[pos++];
  }
  
  if (keyStr.length() == 0) {
    result.errorMessage = "Missing key index";
    return result;
  }
  
  result.keyIndex = keyStr.toInt();
  if (result.keyIndex >= MAX_KEYS) {
    result.errorMessage = "Key index must be 0-" + String(MAX_KEYS - 1);
    return result;
  }
  
  // Skip whitespace
  while (pos < trimmed.length() && isspace(trimmed[pos])) pos++;
  
  if (pos >= trimmed.length()) {
    result.errorMessage = "Missing macro sequence";
    return result;
  }
  
  // Check for down/up event specifier
  String nextWord = "";
  int wordStart = pos;
  while (pos < trimmed.length() && !isspace(trimmed[pos])) {
    nextWord += trimmed[pos++];
  }
  
  nextWord.toLowerCase();
  if (nextWord == "down" || nextWord == "up") {
    result.event = nextWord;
    // Skip whitespace after event
    while (pos < trimmed.length() && isspace(trimmed[pos])) pos++;
  } else {
    result.event = "down"; // Default
    pos = wordStart; // Back up to process this as part of sequence
  }
  
  if (pos >= trimmed.length()) {
    result.errorMessage = "Missing macro sequence";
    return result;
  }
  
  // Parse sequence tokens directly into bytecode
  while (pos < trimmed.length()) {
    // Skip whitespace
    while (pos < trimmed.length() && isspace(trimmed[pos])) pos++;
    if (pos >= trimmed.length()) break;
    
    // Parse next token
    String token = "";
    
    if (trimmed[pos] == '"') {
      // Quoted string
      pos++; // Skip opening quote
      while (pos < trimmed.length() && trimmed[pos] != '"') {
        if (trimmed[pos] == '\\' && pos + 1 < trimmed.length()) {
          token += trimmed[pos++]; // Include backslash
        }
        token += trimmed[pos++];
      }
      if (pos < trimmed.length()) pos++; // Skip closing quote
      
      // Process as literal text with escape sequences
      processEscapeSequences(sequence, token);
      
    } else {
      // Regular token - read until whitespace
      while (pos < trimmed.length() && !isspace(trimmed[pos])) {
        token += trimmed[pos++];
      }
      
      // Analyze token and generate bytecode
      
      // Check for prefix
      bool hasPrefix = false;
      bool isPress = true;
      if (token.startsWith("+")) {
        hasPrefix = true;
        isPress = true;
        token = token.substring(1);
      } else if (token.startsWith("-")) {
        hasPrefix = true;
        isPress = false;
        token = token.substring(1);
      }
      
      // Check for modifier chain
      uint8_t modifierMask = 0;
      String remaining = token;
      String suffixContent = "";
      bool hasMultipleModifiers = false;
      
      while (remaining.length() > 0) {
        int plusPos = remaining.indexOf('+');
        String part;
        
        if (plusPos == -1) {
          part = remaining;
          remaining = "";
        } else {
          part = remaining.substring(0, plusPos);
          remaining = remaining.substring(plusPos + 1);
          hasMultipleModifiers = true; // Found a '+' so this is a chain
        }
        
        uint8_t modBit = findModifierBit(part);
        if (modBit != 0) {
          modifierMask |= modBit;
        } else {
          // Not a modifier - this is suffix content
          suffixContent = part;
          if (remaining.length() > 0) {
            suffixContent += "+" + remaining;
          }
          break;
        }
      }
      
      // Generate bytecode based on what we found
      if (hasPrefix) {
        // Explicit press/release: +CTRL or -CTRL
        if (isPress) {
          addModifierPress(sequence, modifierMask, hasMultipleModifiers);
        } else {
          addModifierRelease(sequence, modifierMask, hasMultipleModifiers);
        }
        
      } else if (modifierMask != 0) {
        // Modifier chain - atomic operation
        addModifierPress(sequence, modifierMask, hasMultipleModifiers);
        
        if (suffixContent.length() > 0) {
          // CTRL+C pattern
          if (suffixContent.length() == 1) {
            sequence += suffixContent[0];
          } else {
            addSpecialKey(sequence, suffixContent);
          }
        } else {
          // CTRL TAB pattern - peek at next token
          int nextPos = pos;
          while (nextPos < trimmed.length() && isspace(trimmed[nextPos])) nextPos++;
          
          if (nextPos < trimmed.length()) {
            String nextToken = "";
            if (trimmed[nextPos] == '"') {
              // Quoted string - apply modifiers to first char only
              nextPos++; // Skip quote
              if (nextPos < trimmed.length() && trimmed[nextPos] != '"') {
                sequence += trimmed[nextPos];
                addModifierRelease(sequence, modifierMask, hasMultipleModifiers);
                
                // Add rest of string without modifiers
                nextPos++;
                String restOfString = "";
                while (nextPos < trimmed.length() && trimmed[nextPos] != '"') {
                  if (trimmed[nextPos] == '\\' && nextPos + 1 < trimmed.length()) {
                    restOfString += trimmed[nextPos++];
                  }
                  restOfString += trimmed[nextPos++];
                }
                if (nextPos < trimmed.length()) nextPos++; // Skip closing quote
                processEscapeSequences(sequence, restOfString);
                pos = nextPos;
                continue;
              }
            } else {
              // Regular token
              while (nextPos < trimmed.length() && !isspace(trimmed[nextPos])) {
                nextToken += trimmed[nextPos++];
              }
              
              if (nextToken.length() == 1) {
                sequence += nextToken[0];
              } else {
                addSpecialKey(sequence, nextToken);
              }
              pos = nextPos;
            }
          }
        }
        
        addModifierRelease(sequence, modifierMask, hasMultipleModifiers);
        
      } else {
        // Not a modifier - check for special key or single char
        if (token.length() == 1) {
          sequence += token[0];
        } else {
          uint8_t directCode, specialCode;
          if (findSpecialKey(token, directCode, specialCode)) {
            if (directCode != 0) {
              sequence += (char)directCode;
            } else {
              sequence += (char)UTF8_SPECIAL_KEY;
              sequence += (char)specialCode;
            }
          } else {
            result.errorMessage = "Unknown token: " + token;
            return result;
          }
        }
      }
    }
  }
  
  result.utf8Sequence = sequence;
  result.valid = true;
  return result;
}

#endif // MAP_PARSER_H
