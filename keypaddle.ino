/*
 * UTF-8+ Enhanced Key Paddle with Chording Support
 * Modified main loop to support both chording and individual key macros
 */

#include <Arduino.h>
#include <Keyboard.h>

#include "config.h"
#include "switches.h"
#include "macro-engine.h"
#include "macro-encode.h"
#include "macro-decode.h"
#include "storage.h"
#include "serial-interface.h"
#include "chording.h"          // New chording support

//==============================================================================
// SYSTEM STATE AND CONSTANTS
//==============================================================================

#define PRESSED 1
#define RELEASED 0

uint32_t lastSwitchState = 0;
bool systemReady = false;

void setup() {
  // Initialize serial communication
  Serial.begin(115200);
  while (!Serial && millis() < 3000) {
    ; // Wait for serial connection (up to 3 seconds)
  }

  Keyboard.begin();
  
  setupSwitches();
  setupStorage();
  setupSerialInterface();
  setupChording();        // Initialize chording system
  
  Serial.println(F("✓ Command interface ready"));
  Serial.println(F("✓ Chording system enabled"));
  
  // Auto-load macros from EEPROM (safe default behavior)
  if (loadFromStorage()) {
    Serial.println(F("✓ Macros loaded from EEPROM"));
  } else {
    Serial.println(F("✓ Default configuration set"));
  }
  
  // Auto-load chords from EEPROM
  if (chording.loadChords()) {
    Serial.println(F("✓ Chords loaded from EEPROM"));
  }
  
  // System ready
  systemReady = true;
  
  // Show startup summary
  Serial.println(F("\n=== System Ready ==="));
  Serial.println(F("Commands: Type HELP for full command list"));
  Serial.println(F("Chording: Press multiple keys simultaneously"));
  Serial.println();
  
  Serial.println(F("Ready for input..."));
}

//==============================================================================
// ARDUINO MAIN LOOP WITH CHORDING SUPPORT
//==============================================================================

void loop() {
  uint32_t currentSwitchState = loopSwitches();
  
  if (currentSwitchState != lastSwitchState) {
    Serial.print("Switches 0x");
    Serial.print(currentSwitchState, HEX);
    Serial.println();
    
    // NEW: Process chording first
    bool chordHandled = processChording(currentSwitchState);
    
    // Only process individual key changes if no chord was handled
    if (!chordHandled) {
      processSwitchChanges(currentSwitchState, lastSwitchState);
    }
    
    lastSwitchState = currentSwitchState;
  }
  
  loopSerialInterface();
}

void processSwitchChanges(uint32_t current, uint32_t previous) {
  if (!systemReady) return;
  
  // Calculate which switches changed
  uint32_t changed = current ^ previous;
  uint32_t pressed = changed & current;    // Newly pressed switches
  uint32_t released = changed & ~current;  // Newly released switches
  
  // Process newly pressed switches
  for (int i = 0; i < MAX_SWITCHES; i++) {
    if (pressed & (1UL << i)) {
      handleKeyEvent(i, PRESSED);
    }
    if (released & (1UL << i)) {
      handleKeyEvent(i, RELEASED);
    }
  }
}

void handleKeyEvent(uint8_t keyIndex, uint8_t event) {
  // Validate key index
  if (keyIndex >= MAX_SWITCHES) {
    return;
  }
  
  // Get the appropriate macro string
  char* macroString = nullptr;
  if (event == PRESSED && macros[keyIndex].downMacro) {
    macroString = macros[keyIndex].downMacro;
  } else if (event == RELEASED && macros[keyIndex].upMacro) {
    macroString = macros[keyIndex].upMacro;
  }
  
  // Execute macro if one exists
  if (macroString && strlen(macroString) > 0) {
    executeUTF8Macro((const uint8_t*)macroString, strlen(macroString));
  }
}