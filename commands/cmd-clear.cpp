/*
 * CLEAR Command Implementation
 * 
 * Clears macro for specified key and direction
 */

#include "cmd-parsing.h"

void cmdClearWithSwitchAndDirection(int switchNum, int direction, const char* remainingArgs) {
  char** target = (direction == DIRECTION_UP) ? &macros[switchNum].upMacro : &macros[switchNum].downMacro;
  
  if (*target) {
    free(*target);
    *target = nullptr;
  }
  Serial.println(F("Cleared"));
}

void cmdClear(const char* args) {
  executeWithSwitchAndDirection(args, cmdClearWithSwitchAndDirection);
}
