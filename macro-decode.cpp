/*
 * Macro Decompiler Implementation
 * 
 * Converts UTF-8+ encoded macro sequences back to human-readable format
 */

#include "map-parser-tables.h"

String macroDecode(const uint8_t* bytes, uint16_t length) {
  if (!bytes || length == 0) return F("\"\"");
  
  String result = "";
  bool needSpace = false;
  
  for (uint16_t i = 0; i < length; ) {
    // Add space between tokens if needed
    if (needSpace && result.length() > 0) {
      result += " ";
    }
    needSpace = true;
    
    uint8_t b = bytes[i];
    
    // Check for control codes first
    switch (b) {
      case UTF8_PRESS_CTRL:    result += "+CTRL"; i++; continue;
      case UTF8_PRESS_ALT:     result += "+ALT"; i++; continue;
      case UTF8_PRESS_SHIFT:   result += "+SHIFT"; i++; continue;
      case UTF8_PRESS_CMD:     result += "+WIN"; i++; continue;
      case UTF8_RELEASE_CTRL:  result += "-CTRL"; i++; continue;
      case UTF8_RELEASE_ALT:   result += "-ALT"; i++; continue;
      case UTF8_RELEASE_SHIFT: result += "-SHIFT"; i++; continue;
      case UTF8_RELEASE_CMD:   result += "-WIN"; i++; continue;
      
      case UTF8_PRESS_MULTI:
        if (i + 1 < length) {
          uint8_t mask = bytes[i + 1];
          result += "+";
          bool first = true;
          if (mask & MULTI_CTRL)  { result += "CTRL"; first = false; }
          if (mask & MULTI_SHIFT) { if (!first) result += "+"; result += "SHIFT"; first = false; }
          if (mask & MULTI_ALT)   { if (!first) result += "+"; result += "ALT"; first = false; }
          if (mask & MULTI_CMD)   { if (!first) result += "+"; result += "WIN"; }
          i += 2;
        } else {
          i++;
        }
        continue;
        
      case UTF8_RELEASE_MULTI:
        if (i + 1 < length) {
          uint8_t mask = bytes[i + 1];
          result += "-";
          bool first = true;
          if (mask & MULTI_CTRL)  { result += "CTRL"; first = false; }
          if (mask & MULTI_SHIFT) { if (!first) result += "+"; result += "SHIFT"; first = false; }
          if (mask & MULTI_ALT)   { if (!first) result += "+"; result += "ALT"; first = false; }
          if (mask & MULTI_CMD)   { if (!first) result += "+"; result += "WIN"; }
          i += 2;
        } else {
          i++;
        }
        continue;
    }
    
    // Check if it's a function key or special key BEFORE checking for regular characters
    const char* keyword = findKeywordForHID(b);
    if (keyword) {
      // Use keyword form for function keys and special keys
      result += keyword;
      i++;
      continue;
    }
    
    // Handle printable characters and control characters as quoted strings
    if (isRegularCharacter(b) || b == ' ' || b == '\n' || b == '\r' || b == '\t' || b == '\a') {
      uint16_t stringStart = i;
      while (i < length && (isRegularCharacter(bytes[i]) || bytes[i] == ' ' || 
                           bytes[i] == '\n' || bytes[i] == '\r' || bytes[i] == '\t' || bytes[i] == '\a')) {
        // Stop if we hit a control code
        if (bytes[i] >= UTF8_PRESS_CTRL && bytes[i] <= UTF8_RELEASE_CMD) break;
        if (bytes[i] == UTF8_PRESS_MULTI || bytes[i] == UTF8_RELEASE_MULTI) break;
        if (bytes[i] == UTF8_ESCAPE || bytes[i] == UTF8_BACKSPACE) break;
        
        // Stop if we hit a function key or special key
        if (findKeywordForHID(bytes[i]) != nullptr) break;
        
        i++;
      }
      
      // Always quote strings
      result += "\"";
      for (uint16_t j = stringStart; j < i; j++) {
        char c = (char)bytes[j];
        switch (c) {
          case '"':  result += "\\\""; break;
          case '\\': result += "\\\\"; break;
          case '\n': result += "\\n"; break;
          case '\r': result += "\\r"; break;
          case '\t': result += "\\t"; break;
          case '\a': result += "\\a"; break;
          default:   result += c; break;
        }
      }
      result += "\"";
      continue;
    }
    
    // Unknown byte - add as hex escape
    result += "\\x";
    if (b < 0x10) result += "0";
    result += String(b, HEX);
    i++;
  }
  
  // Special case: if result is empty, return empty quotes
  if (result.length() == 0) {
    return F("\"\"");
  }
  
  return result;
}