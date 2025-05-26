/*
 * HELP Command Implementation
 * 
 * Shows available commands and their usage
 */

#include "../serial-interface.h"

void cmdHelp() {
  Serial.println(F("\nCommands:"));
  Serial.println(F("HELP - show this help"));
  Serial.println(F("SHOW <key|ALL> [up] - show macro(s)"));
  Serial.println(F("MAP <key> [up] <macro> - set macro"));
  Serial.println(F("CLEAR <key> [up] - clear macro"));
  Serial.println(F("LOAD - load from EEPROM"));
  Serial.println(F("SAVE - save to EEPROM"));
  Serial.println(F("STAT - show status"));
  Serial.println(F("\nKeys: 0-23, direction: down(default) or up"));
}