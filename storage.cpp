/*
 * UTF-8+ Storage System Implementation
 * 
 * EEPROM format: MAX_SWITCHES pairs of \0 terminated strings
 */

#include "config.h"
#include "storage.h"
#include <EEPROM.h>

//==============================================================================
// SHARED DATA STRUCTURE
//==============================================================================

SwitchMacros macros[MAX_SWITCHES];

//==============================================================================
// PRIVATE HELPER FUNCTIONS
//==============================================================================

// Free a macro string if it exists
void freeMacroString(char*& macroPtr) {
  if (macroPtr) {
    free(macroPtr);
    macroPtr = nullptr;
  }
}

// Allocate and copy a macro string (returns nullptr if empty or allocation fails)
char* allocateMacroString(const char* source) {
  if (!source || strlen(source) == 0) {
    return nullptr;
  }
  
  size_t len = strlen(source);
  char* newMacro = (char*)malloc(len + 1);
  if (newMacro) {
    strcpy(newMacro, source);
  }
  return newMacro;
}

// Read a \0 terminated string from EEPROM starting at offset
// Returns new offset after the string, or 0 if error
uint16_t readStringFromEEPROM(uint16_t offset, char* buffer, size_t maxLen) {
  size_t len = 0;
  
  // Read characters until \0 or max length
  while (len < maxLen - 1 && offset < EEPROM.length()) {
    uint8_t ch = EEPROM.read(offset++);
    buffer[len++] = ch;
    if (ch == 0) {
      return offset; // Found terminator, return next offset
    }
  }
  
  // If we get here, no terminator found or buffer full
  buffer[len] = 0; // Ensure null termination
  return (len < maxLen - 1) ? offset : 0; // Return 0 if error
}

// Write a \0 terminated string to EEPROM at offset
// Returns new offset after the string
uint16_t writeStringToEEPROM(uint16_t offset, const char* str) {
  if (!str) {
    // Write empty string (just null terminator)
    EEPROM.write(offset++, 0);
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

//==============================================================================
// PUBLIC INTERFACE FUNCTIONS
//==============================================================================

void setupStorage() {
  // Initialize all switch macros to nullptr
  for (int i = 0; i < MAX_SWITCHES; i++) {
    macros[i].downMacro = nullptr;
    macros[i].upMacro = nullptr;
  }
}

bool loadFromStorage() {
  // Check for magic number
  uint32_t magic;
  EEPROM.get(EEPROM_MAGIC_ADDR, magic);
  
  if (magic != EEPROM_MAGIC_VALUE) {
    // No valid data found, leave switches empty
    return false;
  }
  
  // Clear existing macros
  for (int i = 0; i < MAX_SWITCHES; i++) {
    freeMacroString(macros[i].downMacro);
    freeMacroString(macros[i].upMacro);
  }
  
  // Read 24 pairs of \0 terminated strings
  uint16_t offset = EEPROM_DATA_START;
  char buffer[256]; // Temporary buffer for reading strings
  
  for (int i = 0; i < MAX_SWITCHES; i++) {
    // Read down macro
    offset = readStringFromEEPROM(offset, buffer, sizeof(buffer));
    if (offset == 0) return false; // Read error
    macros[i].downMacro = allocateMacroString(buffer);
    
    // Read up macro  
    offset = readStringFromEEPROM(offset, buffer, sizeof(buffer));
    if (offset == 0) return false; // Read error
    macros[i].upMacro = allocateMacroString(buffer);
  }
  
  return true;
}

bool saveToStorage() {
  // Write magic number
  uint32_t magic = EEPROM_MAGIC_VALUE;
  EEPROM.put(EEPROM_MAGIC_ADDR, magic);
  
  // Write 24 pairs of \0 terminated strings
  uint16_t offset = EEPROM_DATA_START;
  
  for (int i = 0; i < MAX_SWITCHES; i++) {
    // Write down macro (empty string if nullptr)
    offset = writeStringToEEPROM(offset, macros[i].downMacro);
    if (offset >= EEPROM.length()) return false; // Out of space
    
    // Write up macro (empty string if nullptr)
    offset = writeStringToEEPROM(offset, macros[i].upMacro);  
    if (offset >= EEPROM.length()) return false; // Out of space
  }
  
  return true;
}
