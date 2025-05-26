/*
 * CLEAR Command Implementation
 * 
 * Clears macro for specified key and direction
 */

#include "../serial-interface.h"

void cmdClear(const char* args) {
  // Skip leading whitespace
  while (isspace(*args)) args++;
  
  // Parse key number
  char* endptr;
  int key = strtol(args, &endptr, 10);
  if (key < 0 || key >= MAX_SWITCHES || endptr == args) {
    Serial.println(F("Invalid key 0-23"));
    return;
  }
  
  // Check for "up" direction
  bool isUp = false;
  while (isspace(*endptr)) endptr++;
  if (strncasecmp(endptr, "UP", 2) == 0) {
    isUp = true;
  }
  
  char** target = isUp ? &macros[key].upMacro : &macros[key].downMacro;
  
  if (*target) {
    free(*target);
    *target = nullptr;
  }
  Serial.println(F("Cleared"));
}