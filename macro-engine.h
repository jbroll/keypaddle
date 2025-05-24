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

// Execute a UTF-8+ encoded macro sequence
void executeUTF8Macro(const uint8_t* bytes, uint16_t length);

// Initialize the macro execution system (calls Keyboard.begin())
void initializeMacroEngine();

#endif // MACRO_ENGINE_H