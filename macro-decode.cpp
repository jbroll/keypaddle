/*
 * Macro Decompiler Implementation
 * 
 * Converts UTF-8+ encoded macro sequences back to human-readable format
 */

#include "map-parser-tables.h"

String macroDecode(const uint8_t* bytes, uint16_t length) {
  if (!bytes || length == 0) return F("(empty)");
  
  String result = "";
  bool needSpace = false;
  
  for (uint16_t i = 0; i < length; ) {
    // Add space between tokens
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
    
    // Check if it's a keyword using lookup table
    const char* keyword = findKeywordForHID(b);
    if (keyword) {
      result += keyword;
      i++;
      continue;
    }
    
    // Handle regular characters - collect consecutive ones
    if (isRegularCharacter(b)) {
      uint16_t stringStart = i;
      while (i < length && isRegularCharacter(bytes[i])) {
        i++;
      }
      uint16_t stringLength = i - stringStart;
      
      // Use existing utility to check if we need quotes
      bool needsQuotes = (stringLength > 1);
      if (stringLength == 1) {
        needsQuotes = needsQuoting(bytes[stringStart]);
      } else {
        // Check if any character in the string needs quoting
        for (uint16_t j = stringStart; j < i && !needsQuotes; j++) {
          if (needsQuoting(bytes[j])) {
            needsQuotes = true;
          }
        }
      }
      
      if (needsQuotes) {
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
      } else {
        // Single character that doesn't need quoting
        result += (char)bytes[stringStart];
      }
      continue;
    }
    
    // Unknown byte - show as hex
    result += "[0x";
    if (b < 0x10) result += "0";
    result += String(b, HEX);
    result += "]";
    i++;
  }
  
  return result;
}
