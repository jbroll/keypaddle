/*
 * Macro Decompiler Interface
 * 
 * Public interface for converting UTF-8+ sequences to human-readable format
 */

#ifndef MACRO_DECOMPILER_H
#define MACRO_DECOMPILER_H

#include "map-parser-tables.h"

//==============================================================================
// DECOMPILER INTERFACE
//==============================================================================

// Convert UTF-8+ encoded macro back to human-readable format
String decodeUTF8Macro(const uint8_t* bytes, uint16_t length);

#endif // MACRO_DECOMPILER_H