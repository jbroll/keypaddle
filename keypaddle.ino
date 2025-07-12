/*
 * UTF-8+ Enhanced Key Paddle with Chording Support
 * Integrated with unified storage system and comprehensive command interface
 */

#include "usb_strings.h"

#include <Arduino.h>
#include <Keyboard.h>

#include "config.h"
#include "switches.h"
#include "macro-engine.h"
#include "macro-encode.h"
#include "macro-decode.h"
#include "storage.h"
#include "chordStorage.h"      // Unified chord storage interface
#include "chording.h"          // Chording engine
#include "serial-interface.h"

//==============================================================================
// SYSTEM STATE AND CONSTANTS
//==============================================================================

#define PRESSED 1
#define RELEASED 0

uint32_t lastSwitchState = 0;
bool systemReady = false;

//==============================================================================
// SETUP FUNCTION
//==============================================================================

void setup() {
  // Initialize serial communication first
  Serial.begin(115200);
  while (!Serial && millis() < 3000) {
    ; // Wait for serial connection (up to 3 seconds)
  }

  // Initialize core systems
  Keyboard.begin();
  setupSwitches();
  setupStorage();
  setupChording();        // Initialize chording system
  setupSerialInterface();
  
  Serial.println(F("✓ UTF-8+ Key Paddle v2.0"));
  Serial.println(F("✓ Hardware interface ready"));
  Serial.println(F("✓ Command interface ready"));
  Serial.println(F("✓ Chording system enabled"));
  
  // Auto-load configuration from EEPROM
  uint16_t chordOffset = loadFromStorage();
  if (chordOffset > 0) {
    Serial.println(F("✓ Switch macros loaded from EEPROM"));
    
    // Load chords using the unified storage system
    uint32_t modifierMask = loadChords(chordOffset,
                                      [](uint32_t keyMask, const char* macroSequence) -> bool {
                                        return chording.addChord(keyMask, macroSequence);
                                      },
                                      []() {
                                        chording.clearAllChords();
                                      });
    
    if (modifierMask > 0 || chording.getChordCount() > 0) {
      // Update modifier mask in chording system
      chording.clearAllModifiers();
      for (int i = 0; i < NUM_SWITCHES; i++) {
        if (modifierMask & (1UL << i)) {
          chording.setModifierKey(i, true);
        }
      }
      Serial.print(F("✓ Loaded "));
      Serial.print(chording.getChordCount());
      Serial.println(F(" chord patterns from EEPROM"));
      
      if (modifierMask > 0) {
        Serial.print(F("✓ Modifier keys: "));
        Serial.println(formatKeyMask(modifierMask));
      }
    } else {
      Serial.println(F("✓ No chord data found (using defaults)"));
    }
  } else {
    Serial.println(F("✓ No stored configuration found (using defaults)"));
  }
  
  // System ready
  systemReady = true;
  
  // Show startup summary
  Serial.println(F("\n=== System Ready ==="));
  Serial.print(F("Available switches: 0-"));
  Serial.println(NUM_SWITCHES - 1);
  Serial.print(F("Chord patterns: "));
  Serial.println(chording.getChordCount());
  Serial.print(F("Modifier keys: "));
  Serial.println(formatKeyMask(chording.getModifierMask()));
  Serial.println(F("\nCommands: Type HELP for full command list"));
  Serial.println(F("Chording: Press multiple keys simultaneously"));
  Serial.println(F("Individual keys: Single key press/release for macros"));
  Serial.println();
  
  Serial.println(F("Ready for input..."));
}

//==============================================================================
// ARDUINO MAIN LOOP WITH INTEGRATED CHORDING
//==============================================================================

void loop() {
  uint32_t currentSwitchState = loopSwitches();
  
  // Process switch state changes
  if (currentSwitchState != lastSwitchState) {
    Serial.print("Switches 0x");
    Serial.print(currentSwitchState, HEX);
    Serial.println();

    if (systemReady) {
      // Process chording first - gets priority over individual keys
      bool chordHandled = processChording(currentSwitchState);
      
      // Process individual key changes only if no chord was handled
      if (!chordHandled) {
        processSwitchChanges(currentSwitchState, lastSwitchState);
      }
    }
    
    lastSwitchState = currentSwitchState;
  }
  
  // Process serial commands
  loopSerialInterface();
}

//==============================================================================
// INDIVIDUAL KEY PROCESSING (when not handled by chording)
//==============================================================================

void processSwitchChanges(uint32_t current, uint32_t previous) {
  if (!systemReady) return;
  
  // Calculate which switches changed
  uint32_t changed = current ^ previous;
  uint32_t pressed = changed & current;    // Newly pressed switches
  uint32_t released = changed & ~current;  // Newly released switches
  
  // Process newly pressed switches (DOWN events)
  for (int i = 0; i < NUM_SWITCHES; i++) {
    if (pressed & (1UL << i)) {
      handleKeyEvent(i, PRESSED);
    }
  }
  
  // Process newly released switches (UP events)
  for (int i = 0; i < NUM_SWITCHES; i++) {
    if (released & (1UL << i)) {
      handleKeyEvent(i, RELEASED);
    }
  }
}

void handleKeyEvent(uint8_t keyIndex, uint8_t event) {
  // Validate key index
  if (keyIndex >= NUM_SWITCHES) {
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

//==============================================================================
// SYSTEM STATUS AND DIAGNOSTICS
//==============================================================================

void printSystemStatus() {
  Serial.println(F("\n=== System Status ==="));
  
  // Hardware status
  Serial.print(F("Current switch state: 0x"));
  Serial.println(lastSwitchState, HEX);
  
  // Individual macro count
  int macroCount = 0;
  for (int i = 0; i < NUM_SWITCHES; i++) {
    if (macros[i].downMacro || macros[i].upMacro) {
      macroCount++;
    }
  }
  Serial.print(F("Individual key macros: "));
  Serial.println(macroCount);
  
  // Chording status
  Serial.print(F("Chord patterns: "));
  Serial.println(chording.getChordCount());
  
  if (chording.getCurrentChord() != 0) {
    Serial.print(F("Current chord: "));
    Serial.println(formatKeyMask(chording.getCurrentChord()));
  }
  
  Serial.print(F("Modifier keys: "));
  Serial.println(formatKeyMask(chording.getModifierMask()));
  
  // Memory status
  Serial.print(F("Free RAM: ~"));
  #ifdef ESP32
  Serial.print(ESP.getFreeHeap());
  #elif defined(ESP8266)
  Serial.print(ESP.getFreeHeap());
  #elif defined(ARDUINO_ARCH_RP2040)
  Serial.print(rp2040.getFreeHeap());
  #elif defined(__AVR__)
  extern char *__brkval;
  extern char __heap_start;
  Serial.print((char*)SP - (__brkval == 0 ? (char*)&__heap_start : __brkval));
  #else
  Serial.print("unknown");
  #endif
  Serial.println(F(" bytes"));
  
  Serial.println(F("====================="));
}
