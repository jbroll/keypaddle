/*
 * Macro Decompiler Implementation
 * 
 * Converts UTF-8+ encoded macro sequences back to human-readable format
 */

#include "map-parser-tables.h"

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