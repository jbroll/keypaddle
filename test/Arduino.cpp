/*
 * Arduino.cpp - Mock Implementation with Time Control
 * Provides Arduino-specific symbols for testing
 */

#include "Arduino.h"
#include "Serial.cpp"
#include "Keyboard.cpp"
#include "EEPROM.cpp"

//==============================================================================
// ARDUINO MEMORY MANAGEMENT SYMBOLS DEFINITIONS
//==============================================================================

// Define the actual symbols that Arduino code expects
int __heap_start = 0x800;  // Simulated heap start address
int* __brkval = nullptr;   // Simulated current break (no allocation initially)

//==============================================================================
// TIME CONTROL STATIC MEMBERS
//==============================================================================

uint32_t TestTimeControl::currentTime = 0;
bool TestTimeControl::useControlledTime = false;