/*
 * UTF-8+ Enhanced Teensy 2.0 Meta Key Paddle
 * Main Arduino Sketch
 * 
 * A programmable macro keyboard with UTF-8+ encoding, dynamic memory
 * management, and explicit LOAD/STORE workflow. Supports up to 24 keys
 * with efficient memory usage and Unicode text support.
 * 
 * Features:
 * - UTF-8+ direct execution engine (no parsing overhead)
 * - Dynamic malloc-based macro storage (60-80% memory savings)
 * - Human-readable command interface
 * - Native Unicode support for international text
 * - Toggle modifier keys with cross-device compatibility
 * - Explicit LOAD/STORE for safe configuration management
 */

#include <Arduino.h>
#include <Keyboard.h>
#include <EEPROM.h>

// Include all system modules
#include "switches-teensy.h"     // Hardware interface and switch handling
#include "macro-engine.h"        // UTF-8+ encoding/execution engine
#include "storage.h"             // Dynamic memory and EEPROM management
#include "map-parser.h"          // Enhanced MAP command parsing
#include "command-interface.h"   // Serial command processing
#include "system-diagnostics.h"  // Testing and validation functions

//==============================================================================
// SYSTEM STATE
//==============================================================================

uint32_t lastSwitchState = 0;
bool systemReady = false;

//==============================================================================
// ARDUINO SETUP FUNCTION
//==============================================================================

void setup() {
  // Initialize serial communication
  Serial.begin(115200);
  while (!Serial && millis() < 3000) {
    ; // Wait for serial connection (up to 3 seconds)
  }
  
  Serial.println(F("\n=== UTF-8+ Enhanced Meta Key Paddle ==="));
  Serial.println(F("Initializing system..."));
  
  // Initialize hardware switches and interrupts
  setupSwitches();
  Serial.println(F("✓ Hardware initialized (24-key interrupt-driven)"));
  
  // Initialize UTF-8+ macro execution engine
  initializeMacroEngine();
  Serial.println(F("✓ UTF-8+ engine initialized"));
  
  // Initialize storage system
  initializeStorage();
  Serial.println(F("✓ Storage system initialized"));
  
  // Initialize command interface
  initializeCommandInterface();
  Serial.println(F("✓ Command interface ready"));
  
  // Auto-load macros from EEPROM (safe default behavior)
  if (loadMacrosFromEEPROM()) {
    Serial.println(F("✓ Macros loaded from EEPROM"));
  } else {
    Serial.println(F("✓ Default configuration set"));
  }
  
  // System ready
  systemReady = true;
  
  // Show startup summary
  Serial.println(F("\n=== System Ready ==="));
  Serial.println(F("Hardware: 24-key interrupt-driven scanning"));
  Serial.println(F("Software: UTF-8+ direct execution engine"));
  Serial.println(F("Storage: Dynamic malloc + EEPROM persistence"));
  Serial.println(F("Commands: Type HELP for full command list"));
  Serial.println();
  
  // Show current configuration
  showStatus();
  
  // Flash LED to indicate ready
  flashStatusLED(100);
  delay(100);
  flashStatusLED(100);
  
  Serial.println(F("Ready for input..."));
}

//==============================================================================
// ARDUINO MAIN LOOP
//==============================================================================

void loop() {
  // Process switch changes (hardware interrupt-driven)
  uint32_t currentSwitchState = loopSwitches();
  
  // Handle switch state changes
  if (currentSwitchState != lastSwitchState) {
    processSwitchChanges(currentSwitchState, lastSwitchState);
    lastSwitchState = currentSwitchState;
  }
  
  // Process serial commands
  updateCommandInterface();
  
  // Small delay to prevent overwhelming the system
  delay(1);
}

//==============================================================================
// SWITCH PROCESSING FUNCTION
//==============================================================================

void processSwitchChanges(uint32_t current, uint32_t previous) {
  if (!systemReady) return;
  
  // Calculate which switches changed
  uint32_t changed = current ^ previous;
  uint32_t pressed = changed & current;    // Newly pressed switches
  uint32_t released = changed & ~current;  // Newly released switches
  
  // Process newly pressed switches
  for (int i = 0; i < MAX_KEYS; i++) {
    if (pressed & (1UL << i)) {
      handleKeyPress(i, true);
    }
    if (released & (1UL << i)) {
      handleKeyPress(i, false);
    }
  }
}

//==============================================================================
// KEY PRESS HANDLER
//==============================================================================

void handleKeyPress(uint8_t keyIndex, bool pressed) {
  // Validate key index
  if (keyIndex >= config_numKeys || !macroEnabled[keyIndex]) {
    return;
  }
  
  // Execute based on key type and press/release state
  switch (keyTypes[keyIndex]) {
    case KEY_TYPE_TOGGLE:
      // Toggle modifiers only respond to press (not release)
      if (pressed) {
        executeToggleMacro(keyIndex);
      }
      break;
      
    case KEY_TYPE_MOMENTARY:
    default:
      // Momentary keys only respond to press (not release)
      if (pressed) {
        executeMacro(keyIndex);
      }
      break;
      
    case KEY_TYPE_HOLD:
      // Future feature: hold while pressed, release when released
      // For now, treat as momentary
      if (pressed) {
        executeMacro(keyIndex);
      }
      break;
  }
  
  // Optional: Flash LED on any key activity
  if (pressed) {
    setStatusLED(true);
    delay(10);
    setStatusLED(false);
  }
}