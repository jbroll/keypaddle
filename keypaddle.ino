/*
 * UTF-8+ Enhanced Teensy 2.0 Meta Key Paddle
 * Main Arduino Sketch (key-paddle.ino)
 * 
 */

#include <Arduino.h>
#include <Keyboard.h>

#include "config.h"

// System modules using clean interface design
#include "switches.h"     // Hardware interface and switch handling
#include "macro-engine.h"        // UTF-8+ encoding/execution engine
#include "macro-encode.h"        // UTF-8+ to executable conversion
#include "macro-decode.h"        // UTF-8+ to human-readable conversion
#include "storage.h"             // Dynamic memory and EEPROM management
#include "serial-interface.h"    // Serial command processing

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

  Serial.println(F("✓ Command interface ready"));
  
  // Auto-load macros from EEPROM (safe default behavior)
  if (loadFromStorage()) {
    Serial.println(F("✓ Macros loaded from EEPROM"));
  } else {
    Serial.println(F("✓ Default configuration set"));
  }
  
  // System ready
  systemReady = true;
  
  // Show startup summary
  Serial.println(F("\n=== System Ready ==="));
  Serial.println(F("Commands: Type HELP for full command list"));
  Serial.println();
  
  Serial.println(F("Ready for input..."));
}

//==============================================================================
// ARDUINO MAIN LOOP
//==============================================================================

void loop() {
  uint32_t currentSwitchState = loopSwitches();
  
  if (currentSwitchState != lastSwitchState) {
    Serial.print("Switches 0x");
    Serial.print(currentSwitchState, HEX);
    Serial.println();
    processSwitchChanges(currentSwitchState, lastSwitchState);
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
