/*
 * Simple Serial Command Interface for UTF-8+ Key Paddle System
 * 
 * Direct command processing with minimal overhead
 */

#ifndef SERIAL_INTERFACE_H
#define SERIAL_INTERFACE_H

#include <Arduino.h>
#include "macro-encode.h"
#include "macro-decode.h" 
#include "storage.h"
#include "switches.h"

//==============================================================================
// SIMPLE INTERFACE
//==============================================================================

// Setup and loop functions
void setupSerialInterface();
void loopSerialInterface();

// Command processing function (exposed for testing)
void processCommand(const char* cmd);

#endif // SERIAL_INTERFACE_H
