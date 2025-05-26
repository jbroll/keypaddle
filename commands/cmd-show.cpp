/*
 * SHOW Command Implementation
 * 
 * Displays macro content for specified key(s)
 */

#include "cmd-parsing.h"

void cmdShowWithSwitchAndDirection(int switchNum, int direction, const char* remainingArgs) {
  const char* macro = (direction == DIRECTION_UP) ? macros[switchNum].upMacro : macros[switchNum].downMacro;
  
  Serial.print(F("Key "));
  Serial.print(switchNum);
  Serial.print((direction == DIRECTION_UP) ? F(" UP: ") : F(" DOWN: "));
  
  if (macro && strlen(macro) > 0) {
    String readable = macroDecode((const uint8_t*)macro, strlen(macro));
    Serial.println(readable);
  } else {
    Serial.println(F("(empty)"));
  }
}

void cmdShow(const char* args) {
  while (isspace(*args)) args++;
  
  // Check for "ALL" special case
  if (strncasecmp(args, "ALL", 3) == 0 && (args[3] == '\0' || isspace(args[3]))) {
    for (int i = 0; i < MAX_SWITCHES; i++) {
      // Down macro
      Serial.print(i);
      Serial.print(F(" DOWN: "));
      if (macros[i].downMacro && strlen(macros[i].downMacro) > 0) {
        String readable = macroDecode((const uint8_t*)macros[i].downMacro, strlen(macros[i].downMacro));
        Serial.println(readable);
      } else {
        Serial.println();
      }
      
      // Up macro
      Serial.print(i);
      Serial.print(F(" UP: "));
      if (macros[i].upMacro && strlen(macros[i].upMacro) > 0) {
        String readable = macroDecode((const uint8_t*)macros[i].upMacro, strlen(macros[i].upMacro));
        Serial.println(readable);
      } else {
        Serial.println();
      }
    }
    return;
  }
  
  // Regular switch and direction parsing
  executeWithSwitchAndDirection(args, cmdShowWithSwitchAndDirection);
}
