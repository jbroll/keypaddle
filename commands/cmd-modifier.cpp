/*
 * MODIFIER Command Implementation
 * 
 * Manages modifier key designations for chording
 */

#include "../serial-interface.h"
#include "../chording.h"

void cmdModifier(const char* args) {
  while (isspace(*args)) args++;
  
  if (strncasecmp(args, "SET", 3) == 0) {
    args += 3;
    while (isspace(*args)) args++;
    
    // Parse key number
    char* endptr;
    int keyNum = strtol(args, &endptr, 10);
    if (keyNum < 0 || keyNum >= MAX_SWITCHES || endptr == args) {
      Serial.println(F("Invalid key 0-23"));
      return;
    }
    
    if (chording.setModifierKey(keyNum, true)) {
      Serial.print(F("Key "));
      Serial.print(keyNum);
      Serial.println(F(" set as modifier"));
    } else {
      Serial.println(F("Failed to set modifier"));
    }
  }
  else if (strncasecmp(args, "UNSET", 5) == 0) {
    args += 5;
    while (isspace(*args)) args++;
    
    // Parse key number
    char* endptr;
    int keyNum = strtol(args, &endptr, 10);
    if (keyNum < 0 || keyNum >= MAX_SWITCHES || endptr == args) {
      Serial.println(F("Invalid key 0-23"));
      return;
    }
    
    if (chording.setModifierKey(keyNum, false)) {
      Serial.print(F("Key "));
      Serial.print(keyNum);
      Serial.println(F(" unset as modifier"));
    } else {
      Serial.println(F("Failed to unset modifier"));
    }
  }
  else if (strncasecmp(args, "LIST", 4) == 0) {
    Serial.print(F("Modifier keys: "));
    bool first = true;
    for (int i = 0; i < MAX_SWITCHES; i++) {
      if (chording.isModifierKey(i)) {
        if (!first) Serial.print(F(", "));
        Serial.print(i);
        first = false;
      }
    }
    if (first) {
      Serial.print(F("none"));
    }
    Serial.println();
  }
  else if (strncasecmp(args, "CLEAR", 5) == 0) {
    chording.clearAllModifiers();
    Serial.println(F("All modifier keys cleared"));
  }
  else {
    Serial.println(F("Usage:"));
    Serial.println(F("  MODIFIER SET <key>     - Set key as modifier"));
    Serial.println(F("  MODIFIER UNSET <key>   - Unset key as modifier"));
    Serial.println(F("  MODIFIER LIST          - List all modifier keys"));
    Serial.println(F("  MODIFIER CLEAR         - Clear all modifier keys"));
    Serial.println(F(""));
    Serial.println(F("Modifier keys don't need to be released to trigger chords"));
    Serial.println(F("Example: MODIFIER SET 1  (thumb key as shift)"));
  }
}