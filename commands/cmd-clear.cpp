/*
 * CLEAR Command Implementation
 * 
 * Clears macro for specified key and direction
 */

#include "cmd-parsing.h"

void cmdClearWithSwitchAndDirection(int switchNum, int direction, const char* remainingArgs) {
  if (direction == DIRECTION_DOWN || direction == DIRECTION_UNK) {
    // Clear both up and down macros when direction is unknown/ambiguous
    if (macros[switchNum].downMacro) {
      free(macros[switchNum].downMacro);
      macros[switchNum].downMacro = nullptr;
    }
  }
  if (direction == DIRECTION_UP || direction == DIRECTION_UNK) {
    if (macros[switchNum].upMacro) {
      free(macros[switchNum].upMacro);
      macros[switchNum].upMacro = nullptr;
    }
  }
  
  Serial.println(F("Cleared"));
  return;
}

void cmdClear(const char* args) {
  executeWithSwitchAndDirection(args, cmdClearWithSwitchAndDirection);
}
