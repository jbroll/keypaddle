/*
 * MAP Command Parser Implementation - Rewritten for char* efficiency
 * 
 * Parses MAP commands and generates UTF-8+ encoded macro sequences
 * Uses stack-allocated buffer and returns strdup'd result
 */

#include "config.h"
#include "macro-encode.h"

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
  (*input)++; // Skip backslash
  
  if (!**input) {
    // Backslash at end - just add it literally
    return addByte(buffer, pos, '\\');
  }
  
  char next = **input;
  (*input)++; // Skip the escaped character
  
  switch (next) {
    case 'n':  return addByte(buffer, pos, '\n');  // Use actual newline character
    case 'r':  return addByte(buffer, pos, '\r');
    case 't':  return addByte(buffer, pos, '\t');  // Use actual tab character
    case 'a':  return addByte(buffer, pos, '\a');  // Use actual bell character
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
  
  // For old-style syntax like CTRL+C, we need to find the last + and check what follows
  const char* lastPlus = strrchr(token, '+');
  
  if (lastPlus && lastPlus[1] != '\0') {
    // Check if what follows the last + is a modifier
    const char* possibleSuffix = lastPlus + 1;
    uint8_t testBit = findModifierBit(possibleSuffix);
    
    if (testBit == 0) {
      // It's not a modifier, so it's the suffix (like 'C' in CTRL+C)
      strncpy(suffix, possibleSuffix, suffixMaxLen - 1);
      suffix[suffixMaxLen - 1] = '\0';
      
      // Now parse only up to the last +
      int modLen = lastPlus - token;
      char modPart[32];
      if (modLen > 0 && modLen < sizeof(modPart)) {
        strncpy(modPart, token, modLen);
        modPart[modLen] = '\0';
        
        // Parse the modifier part
        const char* pos = modPart;
        char part[32];
        
        while (*pos) {
          int partLen = 0;
          while (*pos && *pos != '+' && partLen < sizeof(part) - 1) {
            part[partLen++] = *pos;
            pos++;
          }
          part[partLen] = '\0';
          
          uint8_t modBit = findModifierBit(part);
          if (modBit != 0) {
            *modifierMask |= modBit;
          }
          
          if (*pos == '+') {
            pos++;
          }
        }
      }
      
      return true;
    }
  }
  
  // No suffix found, parse the whole thing as modifiers
  const char* pos = token;
  char part[32];
  
  while (*pos) {
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

MacroEncodeResult macroEncode(const char* input) {
  MacroEncodeResult result = {nullptr, nullptr};
  
  // Stack-allocated parsing buffer
  uint8_t parseBuffer[MAX_MACRO_LENGTH];
  int bufferPos = 0;
  
  const char* pos = input;
  
  // Skip leading whitespace
  skipWhitespace(&pos);
  
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
        // Check for old-style atomic syntax like CTRL+C
        if (suffix[0] != '\0') {
          // This is an atomic operation like CTRL+C or SHIFT+F1
          if (!addModifierPress(parseBuffer, &bufferPos, modifierMask)) {
            result.error = ERR_BUFFER_OVERFLOW;
            return result;
          }
          
          // Add the key
          if (strlen(suffix) == 1) {
            // Single character - add it directly
            char ch = suffix[0];
            // Convert to lowercase for consistency
            if (ch >= 'A' && ch <= 'Z') {
              ch = ch - 'A' + 'a';
            }
            if (!addByte(parseBuffer, &bufferPos, ch)) {
              result.error = ERR_BUFFER_OVERFLOW;
              return result;
            }
          } else {
            // Multi-character token - must be a keyword
            if (!addKeywordToBuffer(parseBuffer, &bufferPos, suffix)) {
              result.error = ERR_UNKNOWN_TOKEN;
              return result;
            }
          }
          
          // Release modifiers for atomic operation
          if (!addModifierRelease(parseBuffer, &bufferPos, modifierMask)) {
            result.error = ERR_BUFFER_OVERFLOW;
            return result;
          }
        } else {
          // Just modifiers with no suffix - check next token for space-separated pattern
          skipWhitespace(&pos);
          if (*pos != '\0' && *pos != '"') {
            // There's a next token - peek at it
            const char* savedPos = pos;
            char nextToken[64];
            if (parseToken(&pos, nextToken, sizeof(nextToken))) {
              // This is the space-separated pattern like CTRL TAB
              // Add the key
              if (strlen(nextToken) == 1) {
                // Single character
                char ch = nextToken[0];
                // Convert to lowercase for consistency
                if (ch >= 'A' && ch <= 'Z') {
                  ch = ch - 'A' + 'a';
                }
                if (!addByte(parseBuffer, &bufferPos, ch)) {
                  result.error = ERR_BUFFER_OVERFLOW;
                  return result;
                }
              } else {
                // Keyword
                if (!addKeywordToBuffer(parseBuffer, &bufferPos, nextToken)) {
                  result.error = ERR_UNKNOWN_TOKEN;
                  return result;
                }
              }
              
              // Release modifiers for atomic operation
              if (!addModifierRelease(parseBuffer, &bufferPos, modifierMask)) {
                result.error = ERR_BUFFER_OVERFLOW;
                return result;
              }
            } else {
              // No valid next token - just hold the modifiers
              pos = savedPos;
            }
          } else if (*pos == '"') {
            // Quoted string follows - apply modifiers to first char only
            const char* stringStart = pos;
            pos++; // Skip quote
            if (*pos && *pos != '"') {
              // Add first character with modifiers
              char ch = *pos;
              if (!addByte(parseBuffer, &bufferPos, ch)) {
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
              while (*pos && *pos != '"') {
                if (*pos == '\\') {
                  if (!processEscapeSequence(parseBuffer, &bufferPos, &pos)) {
                    result.error = ERR_BUFFER_OVERFLOW;
                    return result;
                  }
                } else {
                  if (!addByte(parseBuffer, &bufferPos, *pos)) {
                    result.error = ERR_BUFFER_OVERFLOW;
                    return result;
                  }
                  pos++;
                }
              }
              
              if (*pos == '"') {
                pos++; // Skip closing quote
              }
              continue;
            } else {
              // Empty string - restore position
              pos = stringStart;
            }
          }
        }
      } else {
        // Not a modifier - check for keyword or single char
        if (strlen(token) == 1) {
          // Single character - add it directly
          char ch = token[0];
          if (!addByte(parseBuffer, &bufferPos, ch)) {
            result.error = ERR_BUFFER_OVERFLOW;
            return result;
          }
        } else {
          // Multi-character token - check if it's a keyword
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