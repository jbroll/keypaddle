/*
 * HELP Command Implementation
 * 
 * Shows available commands and their usage including chording commands
 */

#include "../serial-interface.h"

void cmdHelp() {
  Serial.println(F("\nCommands:"));
  Serial.println(F("=== Individual Key Macros ==="));
  Serial.println(F("HELP - show this help"));
  Serial.println(F("SHOW <key|ALL> [up] - show macro(s)"));
  Serial.println(F("MAP <key> [up] <macro> - set macro"));
  Serial.println(F("CLEAR <key> [up] - clear macro"));
  Serial.println(F("LOAD - load macros from EEPROM"));
  Serial.println(F("SAVE - save macros to EEPROM"));
  
  Serial.println(F("\n=== Chord Management ==="));
  Serial.println(F("CHORD ADD <keys> <macro> - add chord pattern"));
  Serial.println(F("CHORD REMOVE <keys> - remove chord"));
  Serial.println(F("CHORD LIST - list all chords"));
  Serial.println(F("CHORD CLEAR - clear all chords"));
  Serial.println(F("CHORD SAVE - save chords to EEPROM"));
  Serial.println(F("CHORD MODIFIERS [keys] - set/show modifier keys"));
  Serial.println(F("CHORD LOAD - load chords from EEPROM"));
  Serial.println(F("CHORD STATUS - show chording status"));
  
  Serial.println(F("\n=== System ==="));
  Serial.println(F("STAT - show status"));
  
  // FIXED: Use NUM_SWITCHES to show correct key range
  Serial.print(F("\nKeys: 0-"));
  Serial.print(NUM_SWITCHES - 1);
  Serial.println(F(", direction: down(default) or up"));
  Serial.println(F("Chord keys: 0,1,5 or 0+1+5 format"));
  Serial.println(F("Modifier keys don't need release to trigger chords"));
}