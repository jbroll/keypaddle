/*
 * UTF-8+ Storage System
 * 
 * Dynamic memory allocation with explicit LOAD/STORE commands:
 * - RAM: malloc'd UTF-8+ strings for active macros (fast execution)
 * - EEPROM: NUL-delimited strings for persistence 
 * - Explicit control: User decides when to load/store
 * - Minimal overhead: Only allocates memory actually needed
 */

#ifndef STORAGE_H
#define STORAGE_H

#include <Arduino.h>
#include <EEPROM.h>

// Configuration constants
#define MAX_KEYS 24
#define DEFAULT_KEYS 4
#define EEPROM_SIZE 1024
#define EEPROM_MAGIC_ADDR 0
#define EEPROM_CONFIG_ADDR 4
#define EEPROM_DATA_START 64
#define EEPROM_MAGIC_VALUE 0xCAFE2025

// Key behavior types
#define KEY_TYPE_MOMENTARY 0  // Execute macro once per press
#define KEY_TYPE_TOGGLE 1     // Toggle state (for modifiers)
#define KEY_TYPE_HOLD 2       // Hold while pressed (future feature)

// Storage configuration
struct Config {
  uint8_t numKeys;
  uint8_t version;
  uint16_t eepromUsed;     // Bytes used in EEPROM data area
  uint8_t totalMacros;     // Number of active macros
  uint8_t reserved[3];
};

//==============================================================================
// STORAGE STATE
//==============================================================================

// Macro storage - simple pointer table
char* macroPointers[MAX_KEYS];     // malloc'd UTF-8+ strings
uint16_t macroLengths[MAX_KEYS];   // Cached lengths for performance
uint8_t keyTypes[MAX_KEYS];        // Momentary, toggle, or hold
bool macroEnabled[MAX_KEYS];       // Is this macro slot active

// System state
Config config;
bool macrosLoaded = false;
bool macrosDirty = false;          // RAM has changes not saved to EEPROM
uint16_t totalRAMUsed = 0;         // Total malloc'd bytes
uint8_t config_numKeys = DEFAULT_KEYS;  // External interface

//==============================================================================
// MEMORY MANAGEMENT FUNCTIONS
//==============================================================================

bool setMacro(uint8_t keyIndex, const char* utf8String, uint8_t keyType = KEY_TYPE_MOMENTARY) {
  if (keyIndex >= MAX_KEYS) return false;
  
  // Free existing macro if any
  if (macroPointers[keyIndex]) {
    totalRAMUsed -= macroLengths[keyIndex] + 1; // +1 for null terminator
    free(macroPointers[keyIndex]);
    macroPointers[keyIndex] = nullptr;
  }
  
  // Handle empty macro (clear slot)
  if (!utf8String || strlen(utf8String) == 0) {
    macroLengths[keyIndex] = 0;
    keyTypes[keyIndex] = KEY_TYPE_MOMENTARY;
    macroEnabled[keyIndex] = false;
    macrosDirty = true;
    return true;
  }
  
  // Allocate new macro
  uint16_t length = strlen(utf8String);
  char* newMacro = (char*)malloc(length + 1);
  
  if (!newMacro) {
    Serial.println(F("ERROR: Out of RAM for macro"));
    return false;
  }
  
  // Copy string and update tracking
  strcpy(newMacro, utf8String);
  macroPointers[keyIndex] = newMacro;
  macroLengths[keyIndex] = length;
  keyTypes[keyIndex] = keyType;
  macroEnabled[keyIndex] = true;
  totalRAMUsed += length + 1;
  macrosDirty = true;
  
  return true;
}

// Execute macro from RAM
void executeMacro(uint8_t keyIndex) {
  if (keyIndex >= MAX_KEYS || !macroEnabled[keyIndex] || !macroPointers[keyIndex]) {
    return;
  }
  
  // Execute the UTF-8+ encoded macro
  executeUTF8Macro((uint8_t*)macroPointers[keyIndex], macroLengths[keyIndex]);
}

// Execute toggle modifier from RAM
void executeToggleMacro(uint8_t keyIndex) {
  if (keyIndex >= MAX_KEYS || !macroEnabled[keyIndex] || !macroPointers[keyIndex]) {
    return;
  }
  
  // For toggle modifiers, the macro should contain a single modifier code
  if (macroLengths[keyIndex] == 1) {
    uint8_t modifierCode = (uint8_t)macroPointers[keyIndex][0];
    executeToggleModifier(modifierCode);
  }
}

// Get macro as human-readable string for display
String getMacroDisplay(uint8_t keyIndex) {
  if (keyIndex >= MAX_KEYS || !macroEnabled[keyIndex] || !macroPointers[keyIndex]) {
    return F("(empty)");
  }
  
  return decodeUTF8Macro((uint8_t*)macroPointers[keyIndex], macroLengths[keyIndex]);
}

//==============================================================================
// EEPROM PERSISTENCE
//==============================================================================

// Load all macros from EEPROM to RAM
bool loadMacrosFromEEPROM() {
  Serial.print(F("Loading macros from EEPROM..."));
  
  // Check magic number
  uint32_t magic;
  EEPROM.get(EEPROM_MAGIC_ADDR, magic);
  
  if (magic != EEPROM_MAGIC_VALUE) {
    Serial.println(F(" No valid data found"));
    setDefaultMacros();
    return false;
  }
  
  // Load configuration
  EEPROM.get(EEPROM_CONFIG_ADDR, config);
  config_numKeys = config.numKeys;
  
  // Clear existing macros
  clearAllMacros();
  
  // Read macros from EEPROM
  uint16_t offset = EEPROM_DATA_START;
  uint8_t macrosLoaded = 0;
  
  for (int keyIndex = 0; keyIndex < MAX_KEYS && offset < EEPROM_SIZE - 1; keyIndex++) {
    // Read string length first (look for null terminator)
    uint16_t startOffset = offset;
    uint16_t length = 0;
    
    while (offset < EEPROM_SIZE && EEPROM.read(offset) != 0) {
      offset++;
      length++;
    }
    
    if (length > 0) {
      // Allocate and read macro
      char* macro = (char*)malloc(length + 1);
      if (macro) {
        for (int i = 0; i < length; i++) {
          macro[i] = EEPROM.read(startOffset + i);
        }
        macro[length] = 0; // Null terminator
        
        // Read key type (stored after the null terminator)
        uint8_t keyType = EEPROM.read(offset + 1);
        
        macroPointers[keyIndex] = macro;
        macroLengths[keyIndex] = length;
        keyTypes[keyIndex] = keyType;
        macroEnabled[keyIndex] = true;
        totalRAMUsed += length + 1;
        macrosLoaded++;
      }
    }
    
    // Move past null terminator and key type byte
    offset += 2;
  }
  
  macrosLoaded = true;
  macrosDirty = false;
  
  Serial.print(F(" "));
  Serial.print(macrosLoaded);
  Serial.println(F(" macros loaded"));
  
  return true;
}

// Store all RAM macros to EEPROM
bool storeMacrosToEEPROM() {
  Serial.print(F("Storing macros to EEPROM..."));
  
  // Write magic number
  uint32_t magic = EEPROM_MAGIC_VALUE;
  EEPROM.put(EEPROM_MAGIC_ADDR, magic);
  
  // Count active macros and calculate space needed
  uint8_t activeMacros = 0;
  uint16_t spaceNeeded = 0;
  
  for (int i = 0; i < MAX_KEYS; i++) {
    if (macroEnabled[i] && macroPointers[i]) {
      activeMacros++;
      spaceNeeded += macroLengths[i] + 2; // +2 for null terminator and key type
    }
  }
  
  if (spaceNeeded > EEPROM_SIZE - EEPROM_DATA_START) {
    Serial.println(F(" ERROR: Not enough EEPROM space"));
    return false;
  }
  
  // Write macros to EEPROM
  uint16_t offset = EEPROM_DATA_START;
  
  for (int keyIndex = 0; keyIndex < MAX_KEYS; keyIndex++) {
    if (macroEnabled[keyIndex] && macroPointers[keyIndex]) {
      // Write macro string
      for (int i = 0; i < macroLengths[keyIndex]; i++) {
        EEPROM.write(offset++, macroPointers[keyIndex][i]);
      }
      
      // Write null terminator
      EEPROM.write(offset++, 0);
      
      // Write key type
      EEPROM.write(offset++, keyTypes[keyIndex]);
    }
  }
  
  // Update and write configuration
  config.numKeys = config_numKeys;
  config.version = 1;
  config.eepromUsed = offset - EEPROM_DATA_START;
  config.totalMacros = activeMacros;
  
  EEPROM.put(EEPROM_CONFIG_ADDR, config);
  
  macrosDirty = false;
  
  Serial.print(F(" "));
  Serial.print(activeMacros);
  Serial.print(F(" macros stored ("));
  Serial.print(config.eepromUsed);
  Serial.println(F(" bytes)"));
  
  return true;
}

//==============================================================================
// UTILITY FUNCTIONS
//==============================================================================

// Clear all macros from RAM
void clearAllMacros() {
  for (int i = 0; i < MAX_KEYS; i++) {
    if (macroPointers[i]) {
      free(macroPointers[i]);
      macroPointers[i] = nullptr;
    }
    macroLengths[i] = 0;
    keyTypes[i] = KEY_TYPE_MOMENTARY;
    macroEnabled[i] = false;
  }
  totalRAMUsed = 0;
  macrosDirty = true;
}

// Set default macros (simple modifier keys)
void setDefaultMacros() {
  Serial.println(F("Setting default macros..."));
  
  clearAllMacros();
  
  // Set up default 4-key modifier paddle
  uint8_t defaultMacros[][2] = {
    {UTF8_SHIFT_PRESS, 0},    // SHIFT key
    {UTF8_CTRL_NEXT, 0},      // CTRL modifier
    {UTF8_ALT_NEXT, 0},       // ALT modifier
    {UTF8_GUI_NEXT, 0}        // WIN modifier
  };
  
  const char* descriptions[] = {"SHIFT", "CTRL", "ALT", "WIN"};
  
  for (int i = 0; i < DEFAULT_KEYS; i++) {
    char* macro = (char*)malloc(2);
    if (macro) {
      macro[0] = defaultMacros[i][0];
      macro[1] = 0; // Null terminator
      
      macroPointers[i] = macro;
      macroLengths[i] = 1;
      keyTypes[i] = KEY_TYPE_TOGGLE;
      macroEnabled[i] = true;
      totalRAMUsed += 2;
      
      Serial.print(F("Key "));
      Serial.print(i);
      Serial.print(F(" = "));
      Serial.println(descriptions[i]);
    }
  }
  
  config_numKeys = DEFAULT_KEYS;
  macrosLoaded = true;
  macrosDirty = true;
}

// Show current macro configuration
void showMacros() {
  Serial.println(F("\n=== Current Macro Configuration ==="));
  Serial.print(F("Active keys: "));
  Serial.print(config_numKeys);
  Serial.print(F("/"));
  Serial.println(MAX_KEYS);
  
  uint8_t activeMacros = 0;
  
  for (int i = 0; i < config_numKeys; i++) {
    Serial.print(F("Key "));
    if (i < 10) Serial.print(F(" "));
    Serial.print(i);
    Serial.print(F(": "));
    
    if (macroEnabled[i] && macroPointers[i]) {
      Serial.print(getMacroDisplay(i));
      Serial.print(F(" ("));
      Serial.print(macroLengths[i]);
      Serial.print(F(" bytes, "));
      
      switch (keyTypes[i]) {
        case KEY_TYPE_TOGGLE:
          Serial.print(F("toggle"));
          break;
        case KEY_TYPE_HOLD:
          Serial.print(F("hold"));
          break;
        default:
          Serial.print(F("momentary"));
          break;
      }
      
      Serial.println(F(")"));
      activeMacros++;
    } else {
      Serial.println(F("(empty)"));
    }
  }
  
  Serial.print(F("Total: "));
  Serial.print(activeMacros);
  Serial.println(F(" active macros"));
  Serial.println(F("==================================\n"));
}

// Show memory and system status
void showStatus() {
  Serial.println(F("\n=== System Status ==="));
  
  // Memory usage
  Serial.print(F("RAM usage: "));
  Serial.print(totalRAMUsed);
  Serial.println(F(" bytes (malloc'd strings)"));
  
  Serial.print(F("EEPROM usage: "));
  if (macrosLoaded) {
    Serial.print(config.eepromUsed);
    Serial.print(F("/"));
    Serial.print(EEPROM_SIZE - EEPROM_DATA_START);
    Serial.print(F(" bytes ("));
    Serial.print((float)config.eepromUsed / (EEPROM_SIZE - EEPROM_DATA_START) * 100, 1);
    Serial.println(F("%)"));
  } else {
    Serial.println(F("Unknown (not loaded)"));
  }
  
  // Active macros
  uint8_t activeMacros = 0;
  for (int i = 0; i < MAX_KEYS; i++) {
    if (macroEnabled[i]) activeMacros++;
  }
  
  Serial.print(F("Active macros: "));
  Serial.print(activeMacros);
  Serial.print(F("/"));
  Serial.println(MAX_KEYS);
  
  // System state
  Serial.print(F("Macros loaded: "));
  Serial.println(macrosLoaded ? F("YES") : F("NO"));
  
  Serial.print(F("Unsaved changes: "));
  Serial.println(macrosDirty ? F("YES (use STORE to save)") : F("NO"));
  
  Serial.print(F("Active keys: "));
  Serial.println(config_numKeys);
  
  Serial.println(F("====================\n"));
}

// Free individual macro
bool freeMacro(uint8_t keyIndex) {
  if (keyIndex >= MAX_KEYS) return false;
  
  if (macroPointers[keyIndex]) {
    totalRAMUsed -= macroLengths[keyIndex] + 1;
    free(macroPointers[keyIndex]);
    macroPointers[keyIndex] = nullptr;
  }
  
  macroLengths[keyIndex] = 0;
  keyTypes[keyIndex] = KEY_TYPE_MOMENTARY;
  macroEnabled[keyIndex] = false;
  macrosDirty = true;
  
  return true;
}

// Reset to defaults and save
void resetToDefaults() {
  Serial.println(F("Resetting to default configuration..."));
  setDefaultMacros();
  storeMacrosToEEPROM();
  Serial.println(F("Reset complete"));
}

// Check if system is ready for macro execution
bool isSystemReady() {
  return macrosLoaded;
}

// Validate key index
bool isValidKeyIndex(uint8_t keyIndex) {
  return (keyIndex < config_numKeys);
}

// Set number of active keys
void setNumKeys(uint8_t numKeys) {
  if (numKeys > 0 && numKeys <= MAX_KEYS) {
    config_numKeys = numKeys;
    macrosDirty = true;
    Serial.print(F("Active keys set to "));
    Serial.println(numKeys);
  } else {
    Serial.println(F("ERROR: Number of keys must be 1-24"));
  }
}

// Export configuration in human-readable format
void exportConfiguration() {
  Serial.println(F("=== Configuration Export ==="));
  Serial.print(F("KEYS "));
  Serial.println(config_numKeys);
  
  for (int i = 0; i < config_numKeys; i++) {
    if (macroEnabled[i] && macroPointers[i]) {
      Serial.print(F("MAP "));
      Serial.print(i);
      Serial.print(F(" "));
      Serial.print(getMacroDisplay(i));
      Serial.print(F(" "));
      Serial.println(keyTypes[i]);
    }
  }
  
  Serial.println(F("=== End Export ==="));
}

// Initialize storage system
void initializeStorage() {
  // Initialize arrays
  for (int i = 0; i < MAX_KEYS; i++) {
    macroPointers[i] = nullptr;
    macroLengths[i] = 0;
    keyTypes[i] = KEY_TYPE_MOMENTARY;
    macroEnabled[i] = false;
  }
  
  totalRAMUsed = 0;
  macrosLoaded = false;
  macrosDirty = false;
  config_numKeys = DEFAULT_KEYS;
  
  Serial.println(F("Storage system initialized"));
}

// Cleanup function (if needed)
void cleanupStorage() {
  clearAllMacros();
  Serial.println(F("Storage cleanup complete"));
}

// Test memory allocation
void testMemoryAllocation() {
  Serial.println(F("Testing memory allocation..."));
  
  // Test allocating a small macro
  char* testMacro = (char*)malloc(20);
  if (testMacro) {
    strcpy(testMacro, "test macro");
    Serial.print(F("Allocated test macro: "));
    Serial.println(testMacro);
    free(testMacro);
    Serial.println(F("Test allocation: OK"));
  } else {
    Serial.println(F("Test allocation: FAILED"));
  }
  
  // Show available memory
  Serial.print(F("Current RAM usage: "));
  Serial.print(totalRAMUsed);
  Serial.println(F(" bytes"));
}

// Debug system state
void debugStorageState() {
  Serial.println(F("\n=== Storage Debug ==="));
  
  Serial.print(F("Macros loaded: "));
  Serial.println(macrosLoaded ? F("YES") : F("NO"));
  
  Serial.print(F("Dirty flag: "));
  Serial.println(macrosDirty ? F("YES") : F("NO"));
  
  Serial.print(F("Total RAM used: "));
  Serial.print(totalRAMUsed);
  Serial.println(F(" bytes"));
  
  Serial.print(F("Active keys: "));
  Serial.println(config_numKeys);
  
  // Show individual macro pointers
  Serial.println(F("Macro pointer table:"));
  for (int i = 0; i < config_numKeys; i++) {
    Serial.print(F("  Key "));
    Serial.print(i);
    Serial.print(F(": "));
    
    if (macroPointers[i]) {
      Serial.print(F("0x"));
      Serial.print((uintptr_t)macroPointers[i], HEX);
      Serial.print(F(" ("));
      Serial.print(macroLengths[i]);
      Serial.print(F(" bytes) = \""));
      
      // Show first few characters safely
      for (int j = 0; j < min(macroLengths[i], 20); j++) {
        uint8_t b = macroPointers[i][j];
        if (b >= 32 && b <= 126) {
          Serial.write(b);
        } else {
          Serial.print(F("\\x"));
          if (b < 0x10) Serial.print(F("0"));
          Serial.print(b, HEX);
        }
      }
      if (macroLengths[i] > 20) Serial.print(F("..."));
      Serial.println(F("\""));
    } else {
      Serial.println(F("NULL"));
    }
  }
  
  Serial.println(F("=== End Storage Debug ===\n"));
}

#endif // STORAGE_H
