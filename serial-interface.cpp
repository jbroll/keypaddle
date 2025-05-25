/*
 * Simple Serial Command Interface Implementation
 * 
 * Non-blocking character-by-character input with char arrays
 */

#include "serial-interface.h"

//==============================================================================
// CONFIGURATION
//==============================================================================

#define MAX_CMD_LINE 128

//==============================================================================
// COMMAND BUFFER
//==============================================================================

static char commandBuffer[MAX_CMD_LINE];
static int bufferPos = 0;

//==============================================================================
// NON-BLOCKING READLINE
//==============================================================================

// Returns pointer to completed line or nullptr if still reading
const char* readLine() {
  while (Serial.available()) {
    char c = Serial.read();
    
    if (c == '\n' || c == '\r') {
      if (bufferPos > 0) {
        commandBuffer[bufferPos] = '\0';  // Null terminate
        bufferPos = 0;
        return commandBuffer;
      }
    }
    else if (c == '\b' || c == 127) {  // Backspace or DEL
      if (bufferPos > 0) {
        bufferPos--;
        Serial.print(F("\b \b"));  // Erase character on terminal
      }
    }
    else if (c >= 32 && c <= 126 && bufferPos < MAX_CMD_LINE - 1) {  // Printable chars
      commandBuffer[bufferPos++] = c;
      Serial.print(c);  // Echo character
    }
  }
  return nullptr;  // Still reading
}

//==============================================================================
// INDIVIDUAL COMMAND FUNCTIONS
//==============================================================================

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

void cmdClear(const char* args) {
  // Skip leading whitespace
  while (isspace(*args)) args++;
  
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
  
  char** target = isUp ? &macros[key].upMacro : &macros[key].downMacro;
  
  if (*target) {
    free(*target);
    *target = nullptr;
  }
  Serial.println(F("Cleared"));
}

void cmdLoad() {
  if (loadFromStorage()) {
    Serial.println(F("Loaded"));
  } else {
    Serial.println(F("Load failed"));
  }
}

void cmdSave() {
  if (saveToStorage()) {
    Serial.println(F("Saved"));
  } else {
    Serial.println(F("Save failed"));
  }
}

void cmdStat() {
  Serial.print(F("Switches: 0x"));
  Serial.println(loopSwitches(), HEX);
  
  // Free RAM estimate
  extern int __heap_start, *__brkval;
  int v;
  Serial.print(F("Free RAM: ~"));
  Serial.println((int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval));
}

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
    // Pass full original command since MAP parser expects it
    cmdMap(cmd);
  }
  else if (strncasecmp(cmd, "CLEAR", 5) == 0) {
    cmdClear(args);
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
