/*
 * MAP Command Parser Interface
 * 
 * Public interface for parsing MAP commands into UTF-8+ sequences
 */

#ifndef MAP_PARSER_H
#define MAP_PARSER_H

#include "map-parser-tables.h"

//==============================================================================
// PARSER INTERFACE
//==============================================================================

// Main parser function - converts MAP command to UTF-8+ sequence
ParsedMapCommand parseMapCommand(const String& input);

// Parser utility functions (used internally but may be useful elsewhere)
void addModifierPress(String& sequence, uint8_t modifierMask, bool useMulti);
void addModifierRelease(String& sequence, uint8_t modifierMask, bool useMulti);
void processEscapeSequences(String& sequence, const String& text);
bool addKeywordToSequence(String& sequence, const String& keyword);

#endif // MAP_PARSER_H