/*
 * MAP Command Parser Implementation - Simplified following new documentation
 * 
 * Parses MAP commands and generates UTF-8+ encoded macro sequences
 * Uses stack-allocated buffer and returns strdup'd result
 */

#include "config.h"
#include "macro-encode.h"

//==============================================================================
// ERROR MESSAGES IN PROGMEM
//==============================================================================

static const char ERR_MISSING_SEQUENCE[] PROGMEM = "Missing macro sequence";
static const char ERR_BUFFER_OVERFLOW[] PROGMEM = "Macro too long";
static const char ERR_OUT_OF_MEMORY[] PROGMEM = "Out of memory";
static const char ERR_UNKNOWN_TOKEN[] PROGMEM = "Unknown token";
static const char ERR_MISSING_KEY[] PROGMEM = "No key follows modifier combination";
static const char ERR_EMPTY_MODIFIER[] PROGMEM = "Empty modifier specification";
static const char ERR_UNKNOWN_MODIFIER[] PROGMEM = "Unknown modifier name";

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
    case 'n':  return addByte(buffer, pos, '\n');
    case 'r':  return addByte(buffer, pos, '\r');
    case 't':  return addByte(buffer, pos, '\t');
    case 'a':  return addByte(buffer, pos, '\a');
    case 'e':  return addByte(buffer, pos, 0x1B);  // Escape character
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

static bool addKeyToBuffer(uint8_t* buffer, int* pos, const char* keyToken) {
  if (strlen(keyToken) == 1) {
    // Single character - convert to lowercase for consistency
    char ch = keyToken[0];
    if (ch >= 'A' && ch <= 'Z') {
      ch = ch - 'A' + 'a';
    }
    return addByte(buffer, pos, ch);
  } else {
    // Multi-character - must be keyword
    uint8_t hidCode = findHIDCodeForKeyword(keyToken);
    if (hidCode != 0) {
      return addByte(buffer, pos, hidCode);
    }
  }
  return false;
}

static uint8_t parseModifierMask(const char* modifierString) {
  uint8_t mask = 0;
  const char* pos = modifierString;
  char part[16];
  
  while (*pos) {
    int partLen = 0;
    // Read until + or end
    while (*pos && *pos != '+' && partLen < sizeof(part) - 1) {
      part[partLen++] = *pos;
      pos++;
    }
    part[partLen] = '\0';
    
    if (partLen == 0) {
      return 0; // Empty part
    }
    
    uint8_t modBit = findModifierBit(part);
    if (modBit == 0) {
      return 0; // Unknown modifier
    }
    
    mask |= modBit;
    
    if (*pos == '+') {
      pos++;
    }
  }
  
  return mask;
}

static bool isModifierToken(const char* token) {
  // Check if token contains only valid modifiers separated by +
  return parseModifierMask(token) != 0;
}

static bool peekNextToken(const char* pos, char* nextToken, int maxLen) {
  skipWhitespace(&pos);
  if (*pos == '\0' || *pos == '"') {
    return false; // No next token or it's a quoted string
  }
  
  return parseToken(&pos, nextToken, maxLen);
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
      // Quoted string - process directly
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
      
      // Determine token type based on prefix
      if (token[0] == '+') {
        // Press and hold operation: +CTRL or +CTRL+SHIFT
        if (strlen(token) == 1) {
          result.error = ERR_EMPTY_MODIFIER;
          return result;
        }
        
        const char* modifierPart = token + 1; // Skip the +
        uint8_t modifierMask = parseModifierMask(modifierPart);
        
        if (modifierMask == 0) {
          result.error = ERR_UNKNOWN_MODIFIER;
          return result;
        }
        
        if (!addModifierPress(parseBuffer, &bufferPos, modifierMask)) {
          result.error = ERR_BUFFER_OVERFLOW;
          return result;
        }
        
      } else if (token[0] == '-') {
        // Release operation: -CTRL or -CTRL+SHIFT
        if (strlen(token) == 1) {
          result.error = ERR_EMPTY_MODIFIER;
          return result;
        }
        
        const char* modifierPart = token + 1; // Skip the -
        uint8_t modifierMask = parseModifierMask(modifierPart);
        
        if (modifierMask == 0) {
          result.error = ERR_UNKNOWN_MODIFIER;
          return result;
        }
        
        if (!addModifierRelease(parseBuffer, &bufferPos, modifierMask)) {
          result.error = ERR_BUFFER_OVERFLOW;
          return result;
        }
        
      } else if (isModifierToken(token)) {
        // Atomic operation: CTRL C or CTRL+SHIFT T
        uint8_t modifierMask = parseModifierMask(token);
        
        // Must have a following key token
        char nextToken[64];
        if (!peekNextToken(pos, nextToken, sizeof(nextToken))) {
          result.error = ERR_MISSING_KEY;
          return result;
        }
        
        // Consume the next token
        skipWhitespace(&pos);
        parseToken(&pos, nextToken, sizeof(nextToken));
        
        // Generate atomic sequence: press modifiers, key, release modifiers
        if (!addModifierPress(parseBuffer, &bufferPos, modifierMask)) {
          result.error = ERR_BUFFER_OVERFLOW;
          return result;
        }
        
        if (!addKeyToBuffer(parseBuffer, &bufferPos, nextToken)) {
          result.error = ERR_UNKNOWN_TOKEN;
          return result;
        }
        
        if (!addModifierRelease(parseBuffer, &bufferPos, modifierMask)) {
          result.error = ERR_BUFFER_OVERFLOW;
          return result;
        }
        
      } else {
        // Regular key or keyword
        if (!addKeyToBuffer(parseBuffer, &bufferPos, token)) {
          result.error = ERR_UNKNOWN_TOKEN;
          return result;
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