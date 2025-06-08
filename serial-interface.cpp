/*
 * Simple Serial Command Interface Implementation
 * 
 * Command processing and dispatch with modular command implementations
 */

#include "serial-interface.h"

//==============================================================================
// READLINE IMPLEMENTATION - Include directly since it's needed
//==============================================================================

#include "commands/readline.cpp"
#include "commands/cmd-parsing.cpp"

//==============================================================================
// INDIVIDUAL COMMAND IMPLEMENTATIONS
//==============================================================================

// Include individual command implementations
#include "commands/cmd-help.cpp"
#include "commands/cmd-show.cpp"
#include "commands/cmd-map.cpp"
#include "commands/cmd-clear.cpp"
#include "commands/cmd-load.cpp"
#include "commands/cmd-chord.cpp"
#include "commands/cmd-save.cpp"
#include "commands/cmd-stat.cpp"


//==============================================================================
// COMMAND PROCESSING
//==============================================================================

void processCommand(const char* cmd) {
  // Skip leading whitespace
  while (isspace(*cmd)) cmd++;
  if (*cmd == '\0') return;
  
  // Find arguments (first space after command)
  const char* args = cmd;
  while (*args && !isspace(*args)) args++;
  while (isspace(*args)) args++;
  
  // Dispatch commands using standard string functions
  if (strncasecmp(cmd, "HELP", 4) == 0) {
    cmdHelp();
  }
  else if (strncasecmp(cmd, "SHOW", 4) == 0) {
    cmdShow(args);
  }
  else if (strncasecmp(cmd, "MAP", 3) == 0) {
    cmdMap(args);
  }
  else if (strncasecmp(cmd, "CLEAR", 5) == 0) {
    cmdClear(args);
  }
  else if (strncasecmp(cmd, "CHORD", 5) == 0) {
    cmdChord(args);
  }
  else if (strncasecmp(cmd, "LOAD", 4) == 0) {
    cmdLoad();
  }
  else if (strncasecmp(cmd, "SAVE", 4) == 0) {
    cmdSave();
  }
  else if (strncasecmp(cmd, "STAT", 4) == 0) {
    cmdStat();
  }
  else {
    Serial.println(F("Unknown command - type HELP"));
  }
}

//==============================================================================
// SETUP AND LOOP
//==============================================================================

void setupSerialInterface() {
  Serial.begin(115200);
  Serial.println(F("\nUTF-8+ Key Paddle v1.0"));
  Serial.println(F("Type HELP for commands"));
  Serial.print(F("keypad> "));
}

void loopSerialInterface() {
  const char* line = readLine();
  if (line != nullptr) {
    Serial.print(F("> "));
    Serial.println(line);
    processCommand(line);
    Serial.print(F("keypad> "));
  }
}
