/*
 * Macro Execution Engine Interface
 * 
 * Public interface for executing UTF-8+ encoded macro sequences
 */

#ifndef MACRO_ENGINE_H
#define MACRO_ENGINE_H

#include "map-parser-tables.h"

//==============================================================================
// EXECUTION ENGINE INTERFACE
//==============================================================================

void executeUTF8Macro(const uint8_t* bytes, uint16_t length);

#endif // MACRO_ENGINE_H
