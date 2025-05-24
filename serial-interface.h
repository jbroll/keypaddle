/*
 * Simple Serial Command Interface for UTF-8+ Key Paddle System
 * 
 * Direct command processing with minimal overhead
 */

#ifndef SERIAL_INTERFACE_H
#define SERIAL_INTERFACE_H

#include <Arduino.h>
#include "storage.h"
#include "map-parser.h"
#include "macro-decompiler.h"
#include "switches-teensy.h"

//==============================================================================
// SIMPLE INTERFACE
//==============================================================================

// Setup and loop functions
void setupSerialInterface();
void loopSerialInterface();

#endif // SERIAL_INTERFACE_H