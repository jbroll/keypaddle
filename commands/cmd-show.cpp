/*
 * SHOW Command Implementation
 * 
 * Displays macro content for specified key(s)
 */

#include "../serial-interface.h"

void cmdShow(const char* args) {
  while (isspace(*args)) args++;
  
  // Check for "ALL"
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
  
  const char* macro = isUp ? macros[key].upMacro : macros[key].downMacro;
  
  Serial.print(F("Key "));
  Serial.print(key);
  Serial.print(isUp ? F(" UP: ") : F(" DOWN: "));
  
  if (macro && strlen(macro) > 0) {
    String readable = macroDecode((const uint8_t*)macro, strlen(macro));
    Serial.println(readable);
  } else {
    Serial.println(F("(empty)"));
  }
}