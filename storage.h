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

extern SwitchMacros macros[MAX_SWITCHES];

//==============================================================================
// STORAGE INTERFACE
//==============================================================================

// Initialize storage system
void setupStorage();

// Load the switch macro pairs from EEPROM into macros[] array
bool loadFromStorage();

// Save the switch macro pairs from macros[] array to EEPROM  
bool saveToStorage();

#endif // STORAGE_H
