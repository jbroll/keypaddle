#include <Arduino.h>
#include "../config.h"
#include "cmd-parsing.h"

bool parseSwitchAndDirection(const char* args, int* switchNum, int* direction, const char** remainingArgs) {
  // Skip leading whitespace
  while (isspace(*args)) args++;
  
  // Parse switch number
  char* endptr;
  int key = strtol(args, &endptr, 10);
  if (key < 0 || key >= NUM_SWITCHES || endptr == args) {
    Serial.println(F("Invalid key 0-23"));
    return false;
  }
  
  *switchNum = key;
  *direction = DIRECTION_UNK;
  
  // Skip whitespace after switch number
  while (isspace(*endptr)) endptr++;
  
  // Check for direction specifier
  if (*endptr != '\0') {
    // Find the end of the current word
    const char* wordEnd = endptr;
    while (*wordEnd && !isspace(*wordEnd)) wordEnd++;
    
    // Check if the word is exactly "UP" or "DOWN" (case insensitive)
    size_t wordLen = wordEnd - endptr;
    if (wordLen == 2 && strncasecmp(endptr, "UP", 2) == 0) {
      *direction = DIRECTION_UP;
      endptr = (char*) wordEnd;
    } else if (wordLen == 4 && strncasecmp(endptr, "DOWN", 4) == 0) {
      *direction = DIRECTION_DOWN;
      endptr = (char*) wordEnd;
    }
  }
  
  // Skip whitespace after direction
  while (isspace(*endptr)) endptr++;
  
  *remainingArgs = endptr;
  return true;
}

void executeWithSwitchAndDirection(const char* args, SwitchDirectionCommandFunc commandFunc) {
  int switchNum, direction;
  const char* remainingArgs;
  
  if (parseSwitchAndDirection(args, &switchNum, &direction, &remainingArgs)) {
    commandFunc(switchNum, direction, remainingArgs);
  }
}
