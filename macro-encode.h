/*
 * MAP Command Parser Interface - Clean char* based design
 * 
 * Parses MAP commands into UTF-8+ sequences using stack allocation
 * and returns strdup'd results for direct assignment to macros[]
 */

#ifndef MAP_PARSER_H
#define MAP_PARSER_H

#include "map-parser-tables.h"

//==============================================================================
// CONFIGURATION
//==============================================================================

#define MAX_MACRO_LENGTH 256

//==============================================================================
// RESULT STRUCTURE
//==============================================================================

struct MacroEncodeResult {
  char* utf8Sequence;        // strdup'd UTF-8+ sequence, caller owns, never nullptr
  const char* error;         // nullptr on success, PROGMEM string on error
};

//==============================================================================
// PARSER INTERFACE
//==============================================================================

// Main parser function - converts macro command to UTF-8+ sequence
// Returns strdup'd result that should be assigned directly to macros[] array
// On error, utf8Sequence is nullptr and error points to PROGMEM string
MacroEncodeResult macroEncode(const char* input);

#endif // MAP_PARSER_H
