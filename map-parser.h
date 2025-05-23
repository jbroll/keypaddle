/*
 * Enhanced MAP Command Parser v2
 * 
 * Implements explicit modifier syntax:
 * - No prefix, dash suffix: CTRL-a (atomic combo)
 * - No prefix, no dash: CTRL TAB (atomic combo with next token)
 * - No prefix, dash multi: CTRL-ALT "xyz" (atomic combo with content)
 * - Plus prefix: +CTRL-ALT "abc" (press mods, send content, leave pressed)
 * - Minus prefix: -CTRL-ALT (release modifiers)
 */

#ifndef MAP_PARSER_H
#define MAP_PARSER_H

#include <Arduino.h>

//==============================================================================
// UTF-8+ CONTROL CODES (Enhanced)
//==============================================================================

// Existing codes (unchanged)
#define UTF8_CTRL_NEXT     0x01  // Press Ctrl with next character
#define UTF8_ALT_NEXT      0x02  // Press Alt with next character  
#define UTF8_GUI_NEXT      0x03  // Press Win/GUI with next character
#define UTF8_SHIFT_PRESS   0x04  // Begin shift mode
#define UTF8_SHIFT_RELEASE 0x05  // End shift mode
#define UTF8_BACKSPACE     0x08  // Backspace key
#define UTF8_TAB           0x09  // Tab key  
#define UTF8_ENTER         0x0A  // Enter key
#define UTF8_ESCAPE        0x0B  // Escape key
#define UTF8_MULTI_MOD     0x0E  // Multi-modifier combination
#define UTF8_SPECIAL_KEY   0x0F  // Special key (F-keys, arrows, etc.)

// Enhanced codes for explicit press/release
#define UTF8_PRESS_CTRL    0x1A  // Press and hold Ctrl
#define UTF8_PRESS_ALT     0x1B  // Press and hold Alt
#define UTF8_PRESS_SHIFT   0x1C  // Press and hold Shift  
#define UTF8_PRESS_GUI     0x1D  // Press and hold GUI
#define UTF8_RELEASE_CTRL  0x1E  // Release Ctrl
#define UTF8_RELEASE_ALT   0x1F  // Release Alt
#define UTF8_RELEASE_GUI   0x19  // Release GUI

//==============================================================================
// LOOKUP TABLES
//==============================================================================

// Modifier information
struct ModifierInfo {
  const char* name;
  uint8_t pressCode;
  uint8_t releaseCode;
  uint8_t comboBit;  // Bit position for multi-modifier mask
};

const ModifierInfo MODIFIERS[] = {
  {"CTRL",  UTF8_PRESS_CTRL,  UTF8_RELEASE_CTRL,  0x01},
  {"ALT",   UTF8_PRESS_ALT,   UTF8_RELEASE_ALT,   0x04}, 
  {"SHIFT", UTF8_PRESS_SHIFT, UTF8_SHIFT_RELEASE, 0x02},
  {"WIN",   UTF8_PRESS_GUI,   UTF8_RELEASE_GUI,   0x08},
  {"GUI",   UTF8_PRESS_GUI,   UTF8_RELEASE_GUI,   0x08}
};
const int NUM_MODIFIERS = 5;

// Special key codes
#define SPECIAL_F1         0x01
#define SPECIAL_F2         0x02
#define SPECIAL_F3         0x03
#define SPECIAL_F4         0x04
#define SPECIAL_F5         0x05
#define SPECIAL_F6         0x06
#define SPECIAL_F7         0x07
#define SPECIAL_F8         0x08
#define SPECIAL_F9         0x09
#define SPECIAL_F10        0x0A
#define SPECIAL_F11        0x0B
#define SPECIAL_F12        0x0C
#define SPECIAL_UP         0x10
#define SPECIAL_DOWN       0x11
#define SPECIAL_LEFT       0x12
#define SPECIAL_RIGHT      0x13
#define SPECIAL_DELETE     0x14
#define SPECIAL_HOME       0x15
#define SPECIAL_END        0x16
#define SPECIAL_PAGE_UP    0x17
#define SPECIAL_PAGE_DOWN  0x18
#define SPECIAL_SPACE      0x20

struct SpecialKeyInfo {
  const char* name;
  uint8_t code;
};

const SpecialKeyInfo SPECIAL_KEYS[] = {
  {"F1",       SPECIAL_F1},     {"F2",       SPECIAL_F2},     {"F3",       SPECIAL_F3},
  {"F4",       SPECIAL_F4},     {"F5",       SPECIAL_F5},     {"F6",       SPECIAL_F6},
  {"F7",       SPECIAL_F7},     {"F8",       SPECIAL_F8},     {"F9",       SPECIAL_F9},
  {"F10",      SPECIAL_F10},    {"F11",      SPECIAL_F11},    {"F12",      SPECIAL_F12},
  {"UP",       SPECIAL_UP},     {"DOWN",     SPECIAL_DOWN},
  {"LEFT",     SPECIAL_LEFT},   {"RIGHT",    SPECIAL_RIGHT},
  {"DELETE",   SPECIAL_DELETE}, {"DEL",      SPECIAL_DELETE},
  {"HOME",     SPECIAL_HOME},   {"END",      SPECIAL_END},
  {"PAGEUP",   SPECIAL_PAGE_UP}, {"PAGEDOWN", SPECIAL_PAGE_DOWN},
  {"ENTER",    UTF8_ENTER},     {"TAB",      UTF8_TAB},
  {"ESC",      UTF8_ESCAPE},    {"BACKSPACE", UTF8_BACKSPACE},
  {"SPACE",    ' '}
};
const int NUM_SPECIAL_KEYS = 25;

//==============================================================================
// PARSING STRUCTURES
//==============================================================================

struct ParsedMapCommand {
  bool valid;
  uint8_t keyIndex;
  String event;          // "down", "up", or "" (default down)
  String utf8Sequence;   // Generated UTF-8+ encoded macro
  String errorMessage;
  
  ParsedMapCommand() : valid(false), keyIndex(0) {}
};

// Token analysis result
struct TokenAnalysis {
  enum Type {
    UNKNOWN,
    QUOTED_STRING,
    MODIFIER_CHAIN,
    SPECIAL_KEY,
    SINGLE_CHAR
  } type;
  
  // For modifier chains
  bool hasPrefix;           // +/- prefix present
  bool isPress;             // true for +, false for -
  bool hasDashSuffix;       // has -suffix
  Vector<const ModifierInfo*> modifiers;
  String suffixContent;     // content after dash (if any)
  
  String content;           // processed content
  
  TokenAnalysis() : type(UNKNOWN), hasPrefix(false), isPress(true), hasDashSuffix(false) {}
};

//==============================================================================
// UTILITY FUNCTIONS
//==============================================================================

const ModifierInfo* findModifier(const String& name) {
  for (int i = 0; i < NUM_MODIFIERS; i++) {
    if (name.equalsIgnoreCase(MODIFIERS[i].name)) {
      return &MODIFIERS[i];
    }
  }
  return nullptr;
}

const SpecialKeyInfo* findSpecialKey(const String& name) {
  for (int i = 0; i < NUM_SPECIAL_KEYS; i++) {
    if (name.equalsIgnoreCase(SPECIAL_KEYS[i].name)) {
      return &SPECIAL_KEYS[i];
    }
  }
  return nullptr;
}

String processEscapeSequences(const String& input) {
  String result = "";
  
  for (int i = 0; i < input.length(); i++) {
    if (input[i] == '\\' && i + 1 < input.length()) {
      char next = input[i + 1];
      switch (next) {
        case 'n':  result += '\n'; i++; break;  // Newline -> ENTER key
        case 'r':  result += '\r'; i++; break;  // Carriage return
        case 't':  result += '\t'; i++; break;  // Tab -> TAB key
        case 'a':  result += '\a'; i++; break;  // Alert/bell
        case '"':  result += '"';  i++; break;  // Literal quote
        case '\\': result += '\\'; i++; break;  // Literal backslash
        default:   result += input[i]; break;   // Unknown escape, keep backslash
      }
    } else {
      result += input[i];
    }
  }
  
  return result;
}

//==============================================================================
// TOKEN ANALYSIS
//==============================================================================

TokenAnalysis analyzeToken(const String& token) {
  TokenAnalysis analysis;
  
  if (token.length() == 0) {
    return analysis;
  }
  
  // Check for quoted string
  if (token.startsWith("\"") && token.endsWith("\"") && token.length() >= 2) {
    analysis.type = TokenAnalysis::QUOTED_STRING;
    String content = token.substring(1, token.length() - 1);
    analysis.content = processEscapeSequences(content);
    return analysis;
  }
  
  // Check for modifier chain patterns
  String workingToken = token;
  
  // Check for +/- prefix
  if (token.startsWith("+")) {
    analysis.hasPrefix = true;
    analysis.isPress = true;
    workingToken = token.substring(1);
  } else if (token.startsWith("-")) {
    analysis.hasPrefix = true;
    analysis.isPress = false;
    workingToken = token.substring(1);
  }
  
  // Look for modifier patterns (contains + or is a single modifier)
  bool hasPlus = workingToken.indexOf('+') != -1;
  bool isSingleModifier = (findModifier(workingToken) != nullptr);
  
  if (hasPlus || isSingleModifier) {
    analysis.type = TokenAnalysis::MODIFIER_CHAIN;
    
    // Parse the modifier chain
    String remaining = workingToken;
    
    while (true) {
      int nextPlus = remaining.indexOf('+');
      if (nextPlus == -1) {
        // Last part - could be modifier or suffix content
        const ModifierInfo* mod = findModifier(remaining);
        if (mod) {
          analysis.modifiers.push_back(mod);
        } else {
          // Not a modifier - this is suffix content
          analysis.hasDashSuffix = true;
          analysis.suffixContent = remaining;
        }
        break;
      }
      
      String modName = remaining.substring(0, nextPlus);
      const ModifierInfo* mod = findModifier(modName);
      if (!mod) {
        // Invalid modifier - not a modifier chain after all
        analysis.type = TokenAnalysis::UNKNOWN;
        return analysis;
      }
      
      analysis.modifiers.push_back(mod);
      remaining = remaining.substring(nextPlus + 1);
    }
    
    return analysis;
  }
  
  // Check for special key
  const SpecialKeyInfo* specialKey = findSpecialKey(token);
  if (specialKey) {
    analysis.type = TokenAnalysis::SPECIAL_KEY;
    analysis.content = String((char)specialKey->code);
    return analysis;
  }
  
  // Check for single character
  if (token.length() == 1) {
    char c = token[0];
    if (c >= 32 && c <= 126) {
      analysis.type = TokenAnalysis::SINGLE_CHAR;
      analysis.content = String(c);
      return analysis;
    }
  }
  
  return analysis; // UNKNOWN type
}

//==============================================================================
// ENHANCED TOKEN PROCESSING
//==============================================================================

// Process a single token into UTF-8+ sequence
String processSingleToken(const TokenAnalysis& analysis) {
  String result = "";
  
  switch (analysis.type) {
    case TokenAnalysis::QUOTED_STRING:
    case TokenAnalysis::SINGLE_CHAR:
      // Convert escape sequences to UTF-8+ codes
      for (int i = 0; i < analysis.content.length(); i++) {
        char c = analysis.content[i];
        switch (c) {
          case '\n': result += (char)UTF8_ENTER; break;
          case '\t': result += (char)UTF8_TAB; break;
          case '\a': break; // Skip bell
          default:
            if (c >= 32 && c <= 126) {
              result += c;
            }
            break;
        }
      }
      break;
      
    case TokenAnalysis::SPECIAL_KEY:
      if (analysis.content[0] >= 0x20) {
        result += analysis.content[0]; // Direct character
      } else if (analysis.content[0] <= 0x1F) {
        result += analysis.content[0]; // Control code
      } else {
        result += (char)UTF8_SPECIAL_KEY;
        result += analysis.content[0];
      }
      break;
      
    case TokenAnalysis::MODIFIER_CHAIN:
      // Handle different modifier chain patterns
      if (analysis.hasPrefix) {
        // Explicit press/release: +CTRL, -CTRL, +CTRL-ALT, -CTRL-ALT
        for (int i = 0; i < analysis.modifiers.size(); i++) {
          if (analysis.isPress) {
            result += (char)analysis.modifiers[i]->pressCode;
          } else {
            result += (char)analysis.modifiers[i]->releaseCode;
          }
        }
      } else {
        // No prefix - this should be handled by processModifierSequence
        // Should not reach here in normal flow
      }
      break;
      
    default:
      break;
  }
  
  return result;
}

// Process modifier sequence with next token(s)
String processModifierSequence(const Vector<TokenAnalysis>& tokens, int startIndex, int& consumedTokens) {
  if (startIndex >= tokens.size()) {
    consumedTokens = 0;
    return "";
  }
  
  const TokenAnalysis& modToken = tokens[startIndex];
  
  if (modToken.type != TokenAnalysis::MODIFIER_CHAIN || modToken.hasPrefix) {
    consumedTokens = 0;
    return "";
  }
  
  String result = "";
  consumedTokens = 1; // At least the modifier token
  
  // Build modifier mask
  uint8_t modMask = 0;
  for (int i = 0; i < modToken.modifiers.size(); i++) {
    modMask |= modToken.modifiers[i]->comboBit;
  }
  
  if (modToken.hasDashSuffix && modToken.suffixContent.length() > 0) {
    // Pattern: CTRL-a or CTRL-ALT (suffix content in same token)
    result += (char)UTF8_MULTI_MOD;
    result += (char)modMask;
    
    // Process suffix content
    if (modToken.suffixContent.length() == 1) {
      char c = modToken.suffixContent[0];
      if (c >= 32 && c <= 126) {
        result += c;
      }
    } else {
      // Multi-character suffix - treat as special key name
      const SpecialKeyInfo* key = findSpecialKey(modToken.suffixContent);
      if (key) {
        if (key->code >= 0x20) {
          result += (char)key->code;
        } else if (key->code <= 0x1F) {
          result += (char)key->code;
        } else {
          result += (char)UTF8_SPECIAL_KEY;
          result += (char)key->code;
        }
      }
    }
  } else if (startIndex + 1 < tokens.size()) {
    // Pattern: CTRL TAB (modifier + next token)
    const TokenAnalysis& nextToken = tokens[startIndex + 1];
    consumedTokens = 2;
    
    result += (char)UTF8_MULTI_MOD;
    result += (char)modMask;
    
    // Process next token
    switch (nextToken.type) {
      case TokenAnalysis::SINGLE_CHAR:
        if (nextToken.content.length() == 1) {
          result += nextToken.content[0];
        }
        break;
        
      case TokenAnalysis::SPECIAL_KEY:
        if (nextToken.content[0] >= 0x20) {
          result += nextToken.content[0];
        } else if (nextToken.content[0] <= 0x1F) {
          result += nextToken.content[0];
        } else {
          result += (char)UTF8_SPECIAL_KEY;
          result += nextToken.content[0];
        }
        break;
        
      case TokenAnalysis::QUOTED_STRING:
        // For quoted strings, we need to apply modifiers to each character
        // This is complex - for now, just add the first character
        if (nextToken.content.length() > 0) {
          char c = nextToken.content[0];
          if (c == '\n') {
            result += (char)UTF8_ENTER;
          } else if (c == '\t') {
            result += (char)UTF8_TAB;
          } else if (c >= 32 && c <= 126) {
            result += c;
          }
          
          // Add remaining characters without modifiers
          for (int i = 1; i < nextToken.content.length(); i++) {
            char ch = nextToken.content[i];
            if (ch == '\n') {
              result += (char)UTF8_ENTER;
            } else if (ch == '\t') {
              result += (char)UTF8_TAB;
            } else if (ch >= 32 && ch <= 126) {
              result += ch;
            }
          }
        }
        break;
        
      default:
        consumedTokens = 1; // Only consume the modifier token
        break;
    }
  }
  
  return result;
}

//==============================================================================
// COMMAND LINE TOKENIZER
//==============================================================================

Vector<String> tokenizeCommand(const String& input) {
  Vector<String> tokens;
  
  int pos = 0;
  while (pos < input.length()) {
    // Skip whitespace
    while (pos < input.length() && isspace(input[pos])) {
      pos++;
    }
    
    if (pos >= input.length()) break;
    
    String token = "";
    
    if (input[pos] == '"') {
      // Quoted string - find closing quote
      token += input[pos++]; // Include opening quote
      
      while (pos < input.length()) {
        char c = input[pos++];
        token += c;
        
        if (c == '"') {
          break; // Found closing quote
        } else if (c == '\\' && pos < input.length()) {
          // Escaped character - include both backslash and next char
          token += input[pos++];
        }
      }
    } else {
      // Regular token - read until whitespace
      while (pos < input.length() && !isspace(input[pos])) {
        token += input[pos++];
      }
    }
    
    if (token.length() > 0) {
      tokens.push_back(token);
    }
  }
  
  return tokens;
}

//==============================================================================
// MAIN MAP COMMAND PARSER
//==============================================================================

ParsedMapCommand parseMapCommand(const String& input) {
  ParsedMapCommand result;
  
  String trimmed = input;
  trimmed.trim();
  
  // Must start with MAP
  if (!trimmed.toUpperCase().startsWith("MAP ")) {
    result.errorMessage = "Command must start with MAP";
    return result;
  }
  
  // Tokenize the command
  Vector<String> rawTokens = tokenizeCommand(trimmed);
  
  if (rawTokens.size() < 3) {
    result.errorMessage = "Usage: MAP <key> [down|up] <sequence>";
    return result;
  }
  
  // Parse key index
  result.keyIndex = rawTokens[1].toInt();
  if (result.keyIndex >= MAX_KEYS) {
    result.errorMessage = "Key index must be 0-" + String(MAX_KEYS - 1);
    return result;
  }
  
  // Check for down/up specifier
  int sequenceStart = 2;
  String eventToken = rawTokens[2];
  eventToken.toLowerCase();
  
  if (eventToken == "down" || eventToken == "up") {
    result.event = eventToken;
    sequenceStart = 3;
    
    if (rawTokens.size() < 4) {
      result.errorMessage = "Missing macro sequence after " + eventToken;
      return result;
    }
  } else {
    result.event = "down"; // Default
  }
  
  // Analyze all sequence tokens
  Vector<TokenAnalysis> analyzedTokens;
  for (int i = sequenceStart; i < rawTokens.size(); i++) {
    TokenAnalysis analysis = analyzeToken(rawTokens[i]);
    if (analysis.type == TokenAnalysis::UNKNOWN) {
      result.errorMessage = "Unknown token: " + rawTokens[i];
      return result;
    }
    analyzedTokens.push_back(analysis);
  }
  
  // Process tokens into UTF-8+ sequence
  String macroSequence = "";
  int tokenIndex = 0;
  
  while (tokenIndex < analyzedTokens.size()) {
    const TokenAnalysis& token = analyzedTokens[tokenIndex];
    
    if (token.type == TokenAnalysis::MODIFIER_CHAIN && !token.hasPrefix) {
      // No prefix modifier chain - use enhanced processing
      int consumed = 0;
      String modResult = processModifierSequence(analyzedTokens, tokenIndex, consumed);
      
      if (consumed > 0) {
        macroSequence += modResult;
        tokenIndex += consumed;
      } else {
        // Fallback to single token processing
        macroSequence += processSingleToken(token);
        tokenIndex++;
      }
    } else {
      // Regular token processing
      macroSequence += processSingleToken(token);
      tokenIndex++;
    }
  }
  
  result.utf8Sequence = macroSequence;
  result.valid = true;
  
  return result;
}

//==============================================================================
// DEBUGGING FUNCTIONS
//==============================================================================

String utf8SequenceToDisplay(const String& sequence) {
  String result = "";
  
  for (int i = 0; i < sequence.length(); i++) {
    uint8_t b = sequence[i];
    
    if (b >= 32 && b <= 126) {
      result += (char)b;
    } else {
      switch (b) {
        case UTF8_PRESS_CTRL:    result += "[+CTRL]"; break;
        case UTF8_PRESS_ALT:     result += "[+ALT]"; break;
        case UTF8_PRESS_SHIFT:   result += "[+SHIFT]"; break;
        case UTF8_PRESS_GUI:     result += "[+WIN]"; break;
        case UTF8_RELEASE_CTRL:  result += "[-CTRL]"; break;
        case UTF8_RELEASE_ALT:   result += "[-ALT]"; break;
        case UTF8_SHIFT_RELEASE: result += "[-SHIFT]"; break;
        case UTF8_RELEASE_GUI:   result += "[-WIN]"; break;
        case UTF8_ENTER:         result += "[ENTER]"; break;
        case UTF8_TAB:           result += "[TAB]"; break;
        case UTF8_ESCAPE:        result += "[ESC]"; break;
        case UTF8_BACKSPACE:     result += "[BACKSPACE]"; break;
        case UTF8_MULTI_MOD:
          result += "[ATOMIC:";
          if (i + 1 < sequence.length()) {
            uint8_t mask = sequence[++i];
            bool needPlus = false;
            if (mask & 0x01) { result += "CTRL"; needPlus = true; }
            if (mask & 0x02) { if (needPlus) result += "+"; result += "SHIFT"; needPlus = true; }
            if (mask & 0x04) { if (needPlus) result += "+"; result += "ALT"; needPlus = true; }
            if (mask & 0x08) { if (needPlus) result += "+"; result += "WIN"; needPlus = true; }
            if (i + 1 < sequence.length()) {
              result += "+";
              uint8_t key = sequence[++i];
              if (key >= 32 && key <= 126) {
                result += (char)key;
              } else {
                result += "0x" + String(key, HEX);
              }
            }
          }
          result += "]";
          break;
        case UTF8_SPECIAL_KEY:
          result += "[SPECIAL:";
          if (i + 1 < sequence.length()) {
            result += "0x" + String(sequence[++i], HEX);
          }
          result += "]";
          break;
        default:
          result += "[0x" + String(b, HEX) + "]";
          break;
      }
    }
  }
  
  return result;
}

void testEnhancedMapParser() {
  Serial.println(F("\n=== Enhanced MAP Parser Tests ==="));
  
  String testCases[] = {
    // Current behavior (unchanged)
    "MAP 1 CTRL-a",                    // atomic CTRL+a
    
    // New explicit syntax
    "MAP 2 CTRL TAB",                  // atomic CTRL+TAB
    "MAP 3 CTRL-ALT \"xyz\"",          // atomic CTRL+ALT+x,y,z (first char gets mods)
    "MAP 4 +CTRL-ALT \"abc\"",         // press CTRL+ALT, type abc, leave pressed
    
    // Additional test cases
    "MAP 5 SHIFT F1",                  // atomic SHIFT+F1
    "MAP 6 +SHIFT \"HELLO\" -SHIFT",   // press shift, type HELLO, release shift
    "MAP 7 ALT-SHIFT-TAB",             // atomic ALT+SHIFT+TAB
    "MAP 8 +CTRL+ALT F4",              // press CTRL+ALT, then F4, leave pressed
    "MAP 9 CTRL \"save file\"",        // atomic CTRL+s, then "ave file"
    "MAP 10 +WIN R \"notepad\" ENTER -WIN", // complex sequence
    
    // Edge cases
    "MAP 11 CTRL",                     // Just modifiers (should work with next token logic)
    "MAP 12 +CTRL",                    // Press and hold CTRL
    "MAP 13 -CTRL",                    // Release CTRL
    "MAP 14 \"hello\" TAB \"world\"",  // Mixed content
    "MAP 15 CTRL-SHIFT \"ABC\""        // Multi-mod with string
  };
  
  for (int i = 0; i < 15; i++) {
    Serial.print(F("Test "));
    Serial.print(i + 1);
    Serial.print(F(": "));
    Serial.println(testCases[i]);
    
    ParsedMapCommand result = parseMapCommand(testCases[i]);
    
    if (result.valid) {
      Serial.print(F("  Key: "));
      Serial.print(result.keyIndex);
      Serial.print(F(", Event: "));
      Serial.print(result.event);
      Serial.print(F(", Sequence: "));
      Serial.println(utf8SequenceToDisplay(result.utf8Sequence));
    } else {
      Serial.print(F("  ERROR: "));
      Serial.println(result.errorMessage);
    }
    Serial.println();
  }
  
  Serial.println(F("=== Enhanced Parser Tests Complete ===\n"));
}

#endif // MAP_PARSER_H
