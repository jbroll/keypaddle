/*
 * UTF-8+ Storage System Implementation - FIXED
 * 
 * EEPROM format: NUM_SWITCHES pairs of \0 terminated strings
 * 
 * FIXES:
 * 1. Consistent handling of empty strings vs null pointers
 * 2. Proper string length validation in writeStringToEEPROM
 * 3. Better error handling in readStringFromEEPROM
 */

#include "config.h"
#include "storage.h"
#include <EEPROM.h>

//==============================================================================
// SHARED DATA STRUCTURE
//==============================================================================

SwitchMacros macros[NUM_SWITCHES];

// Free a macro string if it exists
void freeMacroString(char*& macroPtr) {
  if (macroPtr) {
    free(macroPtr);
    macroPtr = nullptr;
  }
}

// Read a null-terminated string from EEPROM, returns new offset
// Caller must free the returned string
uint16_t readStringFromEEPROM(uint16_t offset, char** str) {
  *str = nullptr;
  
  // Find string length
  uint16_t start = offset;
  while (offset < EEPROM.length() && EEPROM.read(offset) != 0) {
    offset++;
  }
  
  if (offset >= EEPROM.length()) return 0;  // No null terminator found
  
  size_t len = offset - start;
  offset++;  // Skip null terminator
  
  if (len == 0) {
    // Empty string - return null pointer (consistent behavior)
    return offset;
  }
  
  // Allocate and read string
  *str = (char*)malloc(len + 1);
  if (!*str) return 0;  // Allocation failed
  
  for (size_t i = 0; i < len; i++) {
    (*str)[i] = EEPROM.read(start + i);
  }
  (*str)[len] = '\0';
  
  return offset;
}

// Write a \0 terminated string to EEPROM at offset
// Returns new offset after the string
uint16_t writeStringToEEPROM(uint16_t offset, const char* str) {
  if (!str || strlen(str) == 0) {
    // Write empty string (just null terminator) for both null and empty strings
    if (offset < EEPROM.length()) {
      EEPROM.write(offset++, 0);
    }
    return offset;
  }
  
  // Write string including null terminator
  size_t len = strlen(str);
  for (size_t i = 0; i <= len; i++) { // Include null terminator
    if (offset >= EEPROM.length()) break;
    EEPROM.write(offset++, str[i]);
  }
  
  return offset;
}

void setupStorage() {
  // Initialize all switch macros to nullptr
  for (int i = 0; i < NUM_SWITCHES; i++) {
    macros[i].downMacro = nullptr;
    macros[i].upMacro = nullptr;
  }
}

uint16_t loadFromStorage() {
  // Check for magic number
  uint32_t magic;
  EEPROM.get(EEPROM_MAGIC_ADDR, magic);
  
  if (magic != EEPROM_MAGIC_VALUE) {
    // No valid data found, leave switches empty
    return 0;  // Changed from false to 0 for consistency with saveToStorage
  }
  
  // Clear existing macros
  for (int i = 0; i < NUM_SWITCHES; i++) {
    freeMacroString(macros[i].downMacro);
    freeMacroString(macros[i].upMacro);
  }
  
  // Read NUM_SWITCHES pairs of \0 terminated strings
  uint16_t offset = EEPROM_DATA_START;
  char *macro;
  
  for (int i = 0; i < NUM_SWITCHES; i++) {
    // Read down macro
    offset = readStringFromEEPROM(offset, &macro);
    if (offset == 0) return 0; // Read error
    macros[i].downMacro = macro; // Will be nullptr for empty strings
    
    // Read up macro  
    offset = readStringFromEEPROM(offset, &macro);
    if (offset == 0) return 0; // Read error
    macros[i].upMacro = macro; // Will be nullptr for empty strings
  }
  
  return offset;
}

uint16_t saveToStorage() {
  // Write magic number
  uint32_t magic = EEPROM_MAGIC_VALUE;
  EEPROM.put(EEPROM_MAGIC_ADDR, magic);
  
  // Write NUM_SWITCHES pairs of \0 terminated strings
  uint16_t offset = EEPROM_DATA_START;
  
  for (int i = 0; i < NUM_SWITCHES; i++) {
    // Write down macro (empty string if nullptr)
    offset = writeStringToEEPROM(offset, macros[i].downMacro);
    if (offset >= EEPROM.length()) return 0; // Out of space
    
    // Write up macro (empty string if nullptr)
    offset = writeStringToEEPROM(offset, macros[i].upMacro);  
    if (offset >= EEPROM.length()) return 0; // Out of space
  }
  
  return offset;
}