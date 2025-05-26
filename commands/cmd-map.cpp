/*
 * MAP Command Implementation
 * 
 * Sets macro for specified key and direction
 */

#include "../serial-interface.h"

void cmdMap(const char* args) {
  while (isspace(*args)) args++;

  // Parse key number
  char* endptr;
  int key = strtol(args, &endptr, 10);
  if (key < 0 || key >= MAX_SWITCHES || endptr == args) {
    Serial.println(F("Invalid key 0-23"));
    return;
  }
  args = endptr;

  while (isspace(*args)) args++;
  
  // Check for down/up event specifier
  bool isUp = false;
  
  if (strcasecmp(args, "down") == 0) {
      isUp = false;
      args += 4;
      while (isspace(*args)) args++;
  } else if (strcasecmp(args, "up") == 0) {
      isUp = true;
      args += 2;
      while (isspace(*args)) args++;
  }

  MacroEncodeResult parsed = macroEncode(args);

  if (parsed.error != nullptr) {
    Serial.print(F("Parse error: "));
    Serial.println(parsed.error);
    return;
  }
  
  // Free existing macro
  char** target = isUp ? &macros[key].upMacro : &macros[key].downMacro;
  if (*target) {
    free(*target);
  }
  *target = parsed.utf8Sequence;
  
  Serial.println(F("OK"));
}