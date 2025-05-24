/*
 * Refactored MAP Command Parser - Table-Driven Direct HID Codes
 * 
 * Uses the KEYWORD_TABLE from macro-engine.h for consistent compilation
 * Stores HID codes directly in macro bytes (no more SPECIAL key system)
 */

#ifndef MAP_PARSER_H
#define MAP_PARSER_H

#include <Arduino.h>
#include "macro-engine.h"  // For KEYWORD_TABLE and helper functions

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
// MODIFIER LOOKUP TABLE
//==============================================================================

struct ModifierInfo {
  const char* name;
  uint8_t multiBit;
};

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

void addModifierPress(String& sequence, uint8_t modifierMask, bool useMulti) {
  if (modifierMask == 0) return;
  
  // Use multi-modifier encoding for efficiency when multiple modifiers
  int bitCount = 0;
  for (int i = 0; i < 8; i++) {
    if (modifierMask & (1 << i)) bitCount++;
  }
  
  if (bitCount > 1 || useMulti) {
    sequence += (char)UTF8_PRESS_MULTI;
    sequence += (char)modifierMask;
  } else {
    // Single modifier - use individual codes
    if (modifierMask & MULTI_CTRL)  sequence += (char)UTF8_PRESS_CTRL;
    if (modifierMask & MULTI_SHIFT) sequence += (char)UTF8_PRESS_SHIFT;
    if (modifierMask & MULTI_ALT)   sequence += (char)UTF8_PRESS_ALT;
    if (modifierMask & MULTI_CMD)   sequence += (char)UTF8_PRESS_CMD;
  }
}

void addModifierRelease(String& sequence, uint8_t modifierMask, bool useMulti) {
  if (modifierMask == 0) return;
  
  // Use multi-modifier encoding for efficiency when multiple modifiers
  int bitCount = 0;
  for (int i = 0; i < 8; i++) {
    if (modifierMask & (1 << i)) bitCount++;
  }
  
  if (bitCount > 1 || useMulti) {
    sequence += (char)UTF8_RELEASE_MULTI;
    sequence += (char)modifierMask;
  } else {
    // Single modifier - use individual codes
    if (modifierMask & MULTI_CTRL)  sequence += (char)UTF8_RELEASE_CTRL;
    if (modifierMask & MULTI_SHIFT) sequence += (char)UTF8_RELEASE_SHIFT;
    if (modifierMask & MULTI_ALT)   sequence += (char)UTF8_RELEASE_ALT;
    if (modifierMask & MULTI_CMD)   sequence += (char)UTF8_RELEASE_CMD;
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
        case 'a':  i++; break; // Skip bell (no HID equivalent)
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
// SIMPLIFIED SINGLE-PASS PARSER
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
      // Quoted string - process with escape sequences
      pos++; // Skip opening quote
      while (pos < trimmed.length() && trimmed[pos] != '"') {
        if (trimmed[pos] == '\\' && pos + 1 < trimmed.length()) {
          token += trimmed[pos++]; // Include backslash for escape processing
        }
        token += trimmed[pos++];
      }
      if (pos < trimmed.length()) pos++; // Skip closing quote
      
      // Process escape sequences and add directly to sequence
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
      
      // Parse modifier chain
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
          hasMultipleModifiers = true;
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
          // CTRL+C pattern - add the key directly
          if (suffixContent.length() == 1) {
            sequence += suffixContent[0];
          } else {
            // Try to find HID code for keyword
            if (!addKeywordToSequence(sequence, suffixContent)) {
              result.errorMessage = "Unknown key: " + suffixContent;
              return result;
            }
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
                // Use table-driven lookup
                if (!addKeywordToSequence(sequence, nextToken)) {
                  result.errorMessage = "Unknown key: " + nextToken;
                  return result;
                }
              }
              pos = nextPos;
            }
          }
        }
        
        addModifierRelease(sequence, modifierMask, hasMultipleModifiers);
        
      } else {
        // Not a modifier - check for keyword or single char
        if (token.length() == 1) {
          sequence += token[0];
        } else {
          // Use table-driven lookup for keywords
          if (!addKeywordToSequence(sequence, token)) {
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
