/*
 * Macro Decompiler Implementation
 * 
 * Converts UTF-8+ encoded macro sequences back to human-readable format
 */

#include "map-parser-tables.h"

// Helper function to determine if a HID code should remain as keyword
// Returns true for function keys and navigation keys only
bool shouldRemainAsKeyword(uint8_t hidCode) {
  // Function keys: 0x3A-0x45 (F1-F12)
  if (hidCode >= 0x3A && hidCode <= 0x45) {
    return true;
  }
  
  // Navigation keys - only these specific ones
  switch (hidCode) {
    case 0x52: // UP_ARROW
    case 0x51: // DOWN_ARROW  
    case 0x50: // LEFT_ARROW
    case 0x4F: // RIGHT_ARROW
    case 0x4A: // HOME
    case 0x4D: // END
    case 0x4B: // PAGE_UP
    case 0x4E: // PAGE_DOWN
    case 0x4C: // DELETE
      return true;
    default:
      return false;
  }
}

// Helper function to check if a byte is a UTF8+ control code
bool isControlCode(uint8_t b) {
  return (b >= UTF8_PRESS_CTRL && b <= UTF8_RELEASE_CMD) ||
         b == UTF8_PRESS_MULTI || b == UTF8_RELEASE_MULTI;
}

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
    
    // Check if it's a function key or navigation key that should remain as keyword
    const char* keyword = findKeywordForHID(b);
    if (keyword && shouldRemainAsKeyword(b)) {
      // Function keys and navigation keys remain as keywords
      result += keyword;
      i++;
      continue;
    }
    
    // Handle regular characters - group consecutive non-control characters
    uint16_t stringStart = i;
    while (i < length) {
      uint8_t currentByte = bytes[i];
      
      // Stop if we hit a control code
      if (isControlCode(currentByte)) {
        break;
      }
      
      // Stop if we hit a function key or navigation key that should remain as keyword
      const char* currentKeyword = findKeywordForHID(currentByte);
      if (currentKeyword && shouldRemainAsKeyword(currentByte)) {
        break;
      }
      
      // Include this byte in the string
      i++;
    }
    
    // If we have characters to output, quote them
    if (i > stringStart) {
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
          case 0x1B: result += "\\e"; break;  // Escape character
          case 0x08: result += "\\b"; break;  // Backspace character
          default:   
            // Handle other control characters as hex escapes if needed
            if ((uint8_t)c < 0x20 && c != '\n' && c != '\r' && c != '\t' && c != '\a' && c != 0x1B && c != 0x08) {
              result += "\\x";
              if ((uint8_t)c < 0x10) result += "0";
              result += String((uint8_t)c, HEX);
            } else {
              result += c;
            }
            break;
        }
      }
      result += "\"";
    } else {
      // No characters consumed - this shouldn't happen but handle gracefully
      i++;
    }
  }
  
  // Special case: if result is empty, return empty quotes
  if (result.length() == 0) {
    return F("\"\"");
  }
  
  return result;
}