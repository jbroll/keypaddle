/*
 * UTF-8+ Storage System Interface
 * 
 * Manages 24 switches, each with up and down macro strings
 * Simple interface: setup, load, save, and shared data structure
 */

#ifndef STORAGE_H
#define STORAGE_H

#include <Arduino.h>

//==============================================================================
// CONFIGURATION
//==============================================================================

#define MAX_SWITCHES 24
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

// Array of 24 switches, each with up/down macros
extern SwitchMacros switches[MAX_SWITCHES];

//==============================================================================
// STORAGE INTERFACE
//==============================================================================

// Initialize storage system
void setupStorage();

// Load 24 switch macro pairs from EEPROM into switches[] array
bool loadFromStorage();

// Save 24 switch macro pairs from switches[] array to EEPROM  
bool saveToStorage();

#endif // STORAGE_H