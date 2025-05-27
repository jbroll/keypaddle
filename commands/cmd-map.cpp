/*
 * MAP Command Implementation
 * 
 * Sets macro for specified key and direction
 */

#include "cmd-parsing.h"

void cmdMapWithSwitchAndDirection(int switchNum, int direction, const char* remainingArgs) {
  if (direction == DIRECTION_UNK) {
    direction = DIRECTION_DOWN;
  }
  
  MacroEncodeResult parsed = macroEncode(remainingArgs);

  if (parsed.error != nullptr) {
    Serial.print(F("Parse error: "));
    Serial.println(parsed.error);
    return;
  }
  
  // Free existing macro
  char** target = (direction == DIRECTION_UP) ? &macros[switchNum].upMacro : &macros[switchNum].downMacro;
  if (*target) {
    free(*target);
  }
  *target = parsed.utf8Sequence;
  
  Serial.println(F("OK"));
}

void cmdMap(const char* args) {
  executeWithSwitchAndDirection(args, cmdMapWithSwitchAndDirection);
}
