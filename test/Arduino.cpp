/*
 * Arduino.cpp - Mock Implementation
 * Provides Arduino-specific symbols for testing
 */

#include "Arduino.h"

//==============================================================================
// ARDUINO MEMORY MANAGEMENT SYMBOLS DEFINITIONS
//==============================================================================

// Define the actual symbols that Arduino code expects
int __heap_start = 0x800;  // Simulated heap start address
int* __brkval = nullptr;   // Simulated current break (no allocation initially)