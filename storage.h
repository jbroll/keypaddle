/*
 * UTF-8+ Storage System Interface
 * 
 * Manages the macros, each with up and down strings
 */

#ifndef STORAGE_H
#define STORAGE_H

#include <Arduino.h>

#include "config.h"

//==============================================================================
// CONFIGURATION
//==============================================================================

#define EEPROM_MAGIC_VALUE 0xCAFE2025
#define EEPROM_MAGIC_ADDR 0
#define EEPROM_DATA_START 4

//==============================================================================
// SWITCH DATA STRUCTURE
//==============================================================================

struct SwitchMacros {
  char* downMacro;    // UTF-8+ string for switch press (nullptr or \0 = no macro)
  char* upMacro;      // UTF-8+ string for switch release (nullptr or \0 = no macro)
};

//==============================================================================
// SHARED DATA (extern declaration)
//==============================================================================

extern SwitchMacros macros[NUM_SWITCHES];

//==============================================================================
// STORAGE INTERFACE
//==============================================================================

// Initialize storage system
void setupStorage();

// Load the switch macro pairs from EEPROM into macros[] array
uint16_t loadFromStorage();

// Save the switch macro pairs from macros[] array to EEPROM  
uint16_t saveToStorage();

// Write a null-terminated string to EEPROM at offset
// Returns new offset after the string
uint16_t writeStringToEEPROM(uint16_t offset, const char* str);

// Read a null-terminated string from EEPROM, returns new offset
// Caller must free the returned string
uint16_t readStringFromEEPROM(uint16_t offset, char** str);

#endif // STORAGE_H
