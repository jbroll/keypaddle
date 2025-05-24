/*
 * MAP Command Parser Implementation - Rewritten for char* efficiency
 * 
 * Parses MAP commands and generates UTF-8+ encoded macro sequences
 * Uses stack-allocated buffer and returns strdup'd result
 */

#include "config.h"
#include "map-parser.h"

//==============================================================================
// ERROR MESSAGES IN PROGMEM
//==============================================================================

static const char ERR_INVALID_COMMAND[] PROGMEM = "Command must start with MAP";
static const char ERR_MISSING_KEY[] PROGMEM = "Missing key index";
static const char ERR_INVALID_KEY[] PROGMEM = "Invalid key index";
static const char ERR_MISSING_SEQUENCE[] PROGMEM = "Missing macro sequence";
static const char ERR_BUFFER_OVERFLOW[] PROGMEM = "Macro too long";
static const char ERR_OUT_OF_MEMORY[] PROGMEM = "Out of memory";
static const char ERR_UNKNOWN_TOKEN[] PROGMEM = "Unknown token";

//==============================================================================
// PARSER UTILITIES
//==============================================================================

static bool isWhitespace(char c) {
  return c == ' ' || c == '\t' || c == '\r' || c == '\n';
}

static void skipWhitespace(const char** pos) {
  while (**pos && isWhitespace(**pos)) {
    (*pos)++;
  }
}

static bool parseInteger(const char** pos, int* result) {
  const char* start = *pos;
  *result = 0;
  
  if (!**pos || !isdigit(**pos)) {
    return false;
  }
  
  while (**pos && isdigit(**pos)) {
    *result = *result * 10 + (**pos - '0');
    (*pos)++;
  }
  
  return *pos > start;
}

static bool addByte(uint8_t* buffer, int* pos, uint8_t byte) {
  if (*pos >= MAX_MACRO_LENGTH) {
    return false; // Buffer overflow
  }
  buffer[(*pos)++] = byte;
  return true;
}

static bool addModifierPress(uint8_t* buffer, int* pos, uint8_t modifierMask) {
  if (modifierMask == 0) return true;
  
  // Count bits to decide on encoding
  int bitCount = 0;
  for (int i = 0; i < 8; i++) {
    if (modifierMask & (1 << i)) bitCount++;
  }
  
  if (bitCount > 1) {
    // Multi-modifier encoding
    if (!addByte(buffer, pos, UTF8_PRESS_MULTI)) return false;
    if (!addByte(buffer, pos, modifierMask)) return false;
  } else {
    // Single modifier
    if (modifierMask & MULTI_CTRL)  if (!addByte(buffer, pos, UTF8_PRESS_CTRL)) return false;
    if (modifierMask & MULTI_SHIFT) if (!addByte(buffer, pos, UTF8_PRESS_SHIFT)) return false;
    if (modifierMask & MULTI_ALT)   if (!addByte(buffer, pos, UTF8_PRESS_ALT)) return false;
    if (modifierMask & MULTI_CMD)   if (!addByte(buffer, pos, UTF8_PRESS_CMD)) return false;
  }
  return true;
}

static bool addModifierRelease(uint8_t* buffer, int* pos, uint8_t modifierMask) {
  if (modifierMask == 0) return true;
  
  // Count bits to decide on encoding
  int bitCount = 0;
  for (int i = 0; i < 8; i++) {
    if (modifierMask & (1 << i)) bitCount++;
  }
  
  if (bitCount > 1) {
    // Multi-modifier encoding
    if (!addByte(buffer, pos, UTF8_RELEASE_MULTI)) return false;
    if (!addByte(buffer, pos, modifierMask)) return false;
  } else {
    // Single modifier
    if (modifierMask & MULTI_CTRL)  if (!addByte(buffer, pos, UTF8_RELEASE_CTRL)) return false;
    if (modifierMask & MULTI_SHIFT) if (!addByte(buffer, pos, UTF8_RELEASE_SHIFT)) return false;
    if (modifierMask & MULTI_ALT)   if (!addByte(buffer, pos, UTF8_RELEASE_ALT)) return false;
    if (modifierMask & MULTI_CMD)   if (!addByte(buffer, pos, UTF8_RELEASE_CMD)) return false;
  }
  return true;
}

static bool processEscapeSequence(uint8_t* buffer, int* pos, const char** input) {
  char c = **input;
  (*input)++; // Skip backslash
  
  if (!**input) {
    // Backslash at end - just add it literally
    return addByte(buffer, pos, '\\');
  }
  
  char next = **input;
  (*input)++; // Skip the escaped character
  
  switch (next) {
    case 'n':  return addByte(buffer, pos, UTF8_ENTER);
    case 'r':  return addByte(buffer, pos, '\r');
    case 't':  return addByte(buffer, pos, UTF8_TAB);
    case 'a':  return true; // Skip bell (no HID equivalent)
    case '"':  return addByte(buffer, pos, '"');
    case '\\': return addByte(buffer, pos, '\\');
    default:   
      // Unknown escape - add both characters
      if (!addByte(buffer, pos, '\\')) return false;
      return addByte(buffer, pos, next);
  }
}

static bool parseQuotedString(uint8_t* buffer, int* pos, const char** input) {
  (*input)++; // Skip opening quote
  
  while (**input && **input != '"') {
    if (**input == '\\') {
      if (!processEscapeSequence(buffer, pos, input)) {
        return false;
      }
    } else {
      if (!addByte(buffer, pos, **input)) {
        return false;
      }
      (*input)++;
    }
  }
  
  if (**input == '"') {
    (*input)++; // Skip closing quote
  }
  
  return true;
}

static bool parseToken(const char** input, char* token, int maxLen) {
  int len = 0;
  
  while (**input && !isWhitespace(**input) && len < maxLen - 1) {
    token[len++] = **input;
    (*input)++;
  }
  
  token[len] = '\0';
  return len > 0;
}

static bool addKeywordToBuffer(uint8_t* buffer, int* pos, const char* keyword) {
  uint8_t hidCode = findHIDCodeForKeyword(keyword);
  if (hidCode != 0) {
    return addByte(buffer, pos, hidCode);
  }
  return false;
}

static bool parseModifierChain(const char* token, uint8_t* modifierMask, char* suffix, int suffixMaxLen) {
  *modifierMask = 0;
  suffix[0] = '\0';
  
  const char* pos = token;
  char part[32];
  
  while (*pos) {
    // Find next + or end
    int partLen = 0;
    while (*pos && *pos != '+' && partLen < sizeof(part) - 1) {
      part[partLen++] = *pos;
      pos++;
    }
    part[partLen] = '\0';
    
    uint8_t modBit = findModifierBit(part);
    if (modBit != 0) {
      *modifierMask |= modBit;
    } else {
      // Not a modifier - this is suffix content
      strncpy(suffix, part, suffixMaxLen - 1);
      suffix[suffixMaxLen - 1] = '\0';
      
      // Add any remaining content after +
      if (*pos == '+') {
        pos++;
        int suffixLen = strlen(suffix);
        if (suffixLen < suffixMaxLen - 1) {
          suffix[suffixLen++] = '+';
          strncpy(suffix + suffixLen, pos, suffixMaxLen - suffixLen - 1);
          suffix[suffixMaxLen - 1] = '\0';
        }
      }
      break;
    }
    
    if (*pos == '+') {
      pos++;
    }
  }
  
  return true;
}

//==============================================================================
// MAIN PARSER FUNCTION
//==============================================================================

MapParseResult parseMapCommand(const char* input) {
  MapParseResult result = {0, 0, nullptr, nullptr};
  
  // Stack-allocated parsing buffer
  uint8_t parseBuffer[MAX_MACRO_LENGTH];
  int bufferPos = 0;
  
  const char* pos = input;
  
  // Skip leading whitespace
  skipWhitespace(&pos);
  
  // Must start with MAP
  if (strncasecmp(pos, "MAP", 3) != 0 || (pos[3] != '\0' && !isWhitespace(pos[3]))) {
    result.error = ERR_INVALID_COMMAND;
    return result;
  }
  pos += 3;
  
  skipWhitespace(&pos);
  
  // Parse key index
  int keyIndex;
  if (!parseInteger(&pos, &keyIndex)) {
    result.error = ERR_MISSING_KEY;
    return result;
  }
  
  if (keyIndex < 0 || keyIndex >= MAX_SWITCHES) {
    result.error = ERR_INVALID_KEY;
    return result;
  }
  
  result.keyIndex = keyIndex;
  skipWhitespace(&pos);
  
  // Check for down/up event specifier
  result.isUpEvent = false;
  char eventToken[8];
  const char* savedPos = pos;
  
  if (parseToken(&pos, eventToken, sizeof(eventToken))) {
    if (strcasecmp(eventToken, "down") == 0) {
      result.isUpEvent = false;
      skipWhitespace(&pos);
    } else if (strcasecmp(eventToken, "up") == 0) {
      result.isUpEvent = true;
      skipWhitespace(&pos);
    } else {
      // Not an event specifier - restore position
      pos = savedPos;
    }
  }
  
  // Check if we have any macro content
  if (*pos == '\0') {
    result.error = ERR_MISSING_SEQUENCE;
    return result;
  }
  
  // Parse macro tokens
  while (*pos) {
    skipWhitespace(&pos);
    if (*pos == '\0') break;
    
    if (*pos == '"') {
      // Quoted string
      if (!parseQuotedString(parseBuffer, &bufferPos, &pos)) {
        result.error = ERR_BUFFER_OVERFLOW;
        return result;
      }
    } else {
      // Regular token
      char token[64];
      if (!parseToken(&pos, token, sizeof(token))) {
        break;
      }
      
      // Check for prefix
      bool hasPrefix = false;
      bool isPress = true;
      const char* tokenContent = token;
      
      if (token[0] == '+') {
        hasPrefix = true;
        isPress = true;
        tokenContent = token + 1;
      } else if (token[0] == '-') {
        hasPrefix = true;
        isPress = false;
        tokenContent = token + 1;
      }
      
      // Parse modifier chain
      uint8_t modifierMask;
      char suffix[32];
      
      if (!parseModifierChain(tokenContent, &modifierMask, suffix, sizeof(suffix))) {
        result.error = ERR_UNKNOWN_TOKEN;
        return result;
      }
      
      if (hasPrefix) {
        // Explicit press/release: +CTRL or -CTRL
        if (isPress) {
          if (!addModifierPress(parseBuffer, &bufferPos, modifierMask)) {
            result.error = ERR_BUFFER_OVERFLOW;
            return result;
          }
        } else {
          if (!addModifierRelease(parseBuffer, &bufferPos, modifierMask)) {
            result.error = ERR_BUFFER_OVERFLOW;
            return result;
          }
        }
      } else if (modifierMask != 0) {
        // Modifier chain - atomic operation
        if (!addModifierPress(parseBuffer, &bufferPos, modifierMask)) {
          result.error = ERR_BUFFER_OVERFLOW;
          return result;
        }
        
        if (suffix[0] != '\0') {
          // CTRL+C pattern - add the key
          if (strlen(suffix) == 1) {
            if (!addByte(parseBuffer, &bufferPos, suffix[0])) {
              result.error = ERR_BUFFER_OVERFLOW;
              return result;
            }
          } else {
            if (!addKeywordToBuffer(parseBuffer, &bufferPos, suffix)) {
              result.error = ERR_UNKNOWN_TOKEN;
              return result;
            }
          }
        } else {
          // CTRL TAB pattern - peek at next token
          skipWhitespace(&pos);
          if (*pos != '\0') {
            if (*pos == '"') {
              // Quoted string - apply modifiers to first char only
              const char* stringStart = pos;
              pos++; // Skip quote
              if (*pos && *pos != '"') {
                if (!addByte(parseBuffer, &bufferPos, *pos)) {
                  result.error = ERR_BUFFER_OVERFLOW;
                  return result;
                }
                pos++;
                
                // Release modifiers
                if (!addModifierRelease(parseBuffer, &bufferPos, modifierMask)) {
                  result.error = ERR_BUFFER_OVERFLOW;
                  return result;
                }
                
                // Add rest of string without modifiers
                if (!parseQuotedString(parseBuffer, &bufferPos, &pos)) {
                  result.error = ERR_BUFFER_OVERFLOW;
                  return result;
                }
                continue;
              } else {
                // Empty string - restore position
                pos = stringStart;
              }
            } else {
              // Regular token
              char nextToken[64];
              const char* savedPos = pos;
              if (parseToken(&pos, nextToken, sizeof(nextToken))) {
                if (strlen(nextToken) == 1) {
                  if (!addByte(parseBuffer, &bufferPos, nextToken[0])) {
                    result.error = ERR_BUFFER_OVERFLOW;
                    return result;
                  }
                } else {
                  if (!addKeywordToBuffer(parseBuffer, &bufferPos, nextToken)) {
                    result.error = ERR_UNKNOWN_TOKEN;
                    return result;
                  }
                }
              } else {
                // Failed to parse next token - restore position
                pos = savedPos;
              }
            }
          }
        }
        
        if (!addModifierRelease(parseBuffer, &bufferPos, modifierMask)) {
          result.error = ERR_BUFFER_OVERFLOW;
          return result;
        }
      } else {
        // Not a modifier - check for keyword or single char
        if (strlen(token) == 1) {
          if (!addByte(parseBuffer, &bufferPos, token[0])) {
            result.error = ERR_BUFFER_OVERFLOW;
            return result;
          }
        } else {
          if (!addKeywordToBuffer(parseBuffer, &bufferPos, token)) {
            result.error = ERR_UNKNOWN_TOKEN;
            return result;
          }
        }
      }
    }
  }
  
  // Create result string
  char* resultString = (char*)malloc(bufferPos + 1);
  if (!resultString) {
    result.error = ERR_OUT_OF_MEMORY;
    return result;
  }
  
  if (bufferPos > 0) {
    memcpy(resultString, parseBuffer, bufferPos);
  }
  resultString[bufferPos] = '\0';
  
  result.utf8Sequence = resultString;
  return result;
}
