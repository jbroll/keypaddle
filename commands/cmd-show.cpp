/*
 * SHOW Command Implementation
 * 
 * Displays macro content for specified key(s)
 */

#include "cmd-parsing.h"

void printMacro(int switchNum, int direction) {
  if (direction == DIRECTION_DOWN || direction == DIRECTION_UNK) {
    Serial.print(F("Key "));
    Serial.print(switchNum);
    Serial.print(F(" DOWN: "));
    if (macros[switchNum].downMacro && strlen(macros[switchNum].downMacro) > 0) {
      String readable = macroDecode((const uint8_t*)macros[switchNum].downMacro, strlen(macros[switchNum].downMacro));
      Serial.println(readable);
    } else {
      Serial.println(F("(empty)"));
    }
  }
    
  if (direction == DIRECTION_UP || direction == DIRECTION_UNK) {
    Serial.print(F("Key "));
    Serial.print(switchNum);
    Serial.print(F(" UP: "));
    if (macros[switchNum].upMacro && strlen(macros[switchNum].upMacro) > 0) {
      String readable = macroDecode((const uint8_t*)macros[switchNum].upMacro, strlen(macros[switchNum].upMacro));
      Serial.println(readable);
    } else {
      Serial.println(F("(empty)"));
    }
    return;
  }
}

void cmdShowWithSwitchAndDirection(int switchNum, int direction, const char* remainingArgs) {
    printMacro(switchNum, direction);
}

void cmdShow(const char* args) {
  while (isspace(*args)) args++;
  
  // Check for "ALL" special case
  if (strncasecmp(args, "ALL", 3) == 0 && (args[3] == '\0' || isspace(args[3]))) {
    for (int i = 0; i < NUM_SWITCHES; i++) {
        printMacro(i, DIRECTION_UNK);
    }
    return;
  }
  
  // Regular switch and direction parsing
  executeWithSwitchAndDirection(args, cmdShowWithSwitchAndDirection);
}
