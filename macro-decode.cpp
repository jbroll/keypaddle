/*
 * Macro Decompiler Implementation
 * 
 * Converts UTF-8+ encoded macro sequences back to human-readable format
 */

#include "map-parser-tables.h"

// Helper function to get function key name from number
const char* getFunctionKeyName(uint8_t keyNum) {
  switch (keyNum) {
    case 1: return "F1";
    case 2: return "F2";
    case 3: return "F3";
    case 4: return "F4";
    case 5: return "F5";
    case 6: return "F6";
    case 7: return "F7";
    case 8: return "F8";
    case 9: return "F9";
    case 10: return "F10";
    case 11: return "F11";
    case 12: return "F12";
    default: return nullptr;
  }
}

// Helper function to determine if a UTF-8+ code should remain as keyword
// Returns true for function keys and navigation keys only
bool shouldRemainAsKeyword(uint8_t utf8Code) {
  // Function keys are handled separately (2-byte encoding)
  
  // Navigation keys that should remain as keywords
  switch (utf8Code) {
    case UTF8_KEY_UP:
    case UTF8_KEY_DOWN:
    case UTF8_KEY_LEFT:
    case UTF8_KEY_RIGHT:
    case UTF8_KEY_HOME:
    case UTF8_KEY_END:
    case UTF8_KEY_PAGEUP:
    case UTF8_KEY_PAGEDOWN:
    case UTF8_KEY_DELETE:
      return true;
    default:
      return false;
  }
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
        
      case UTF8_FUNCTION_KEY:
        // Handle 2-byte function key encoding
        if (i + 1 < length) {
          uint8_t keyNum = bytes[i + 1];
          const char* keyName = getFunctionKeyName(keyNum);
          if (keyName) {
            result += keyName;
          } else {
            // Invalid function key number - treat as unknown
            result += "F?";
          }
          i += 2;
        } else {
          // Incomplete sequence - skip
          i++;
        }
        continue;
    }
    
    // Check if it's a navigation key that should remain as keyword
    if (shouldRemainAsKeyword(b)) {
      const char* keyword = findKeywordForUTF8Code(b);
      if (keyword) {
        result += keyword;
        i++;
        continue;
      }
    }
    
    // Handle regular characters - group consecutive non-control characters
    uint16_t stringStart = i;
    while (i < length) {
      uint8_t currentByte = bytes[i];
      
      // Stop if we hit any UTF-8+ control code
      if (isUTF8ControlCode(currentByte)) {
        break;
      }
      
      // Stop if we hit a navigation key that should remain as keyword
      if (shouldRemainAsKeyword(currentByte)) {
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