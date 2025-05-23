/*
 * Teensy 2.0 Hardware Interface
 * Simplified switch handling with clean API
 * 
 * Exposes only:
 * - setupSwitches() - Initialize hardware
 * - loopSwitches()  - Returns 32-bit debounced switch state
 * - LED control functions
 */

#ifndef SWITCHES_TEENSY_H
#define SWITCHES_TEENSY_H

#include <Arduino.h>

// Hardware configuration
#define MAX_SWITCHES 24
#define DEBOUNCE_MS 50
#define LED_PIN 13

//==============================================================================
// PUBLIC INTERFACE - Only these functions are exposed
//==============================================================================

void setupSwitches();         // Initialize hardware, called from setup()
uint32_t loopSwitches();      // Process and return debounced switch state
void setStatusLED(bool state); // Control status LED
void flashStatusLED(int duration_ms); // Flash LED for duration

//==============================================================================
// PRIVATE IMPLEMENTATION
//==============================================================================

// Internal hardware state - not exposed to application
static struct {
  // Pin configuration
  uint8_t switchPins[MAX_SWITCHES];
  uint8_t numSwitches;
  
  // Debouncing state
  uint32_t currentState;     // Raw current state from interrupts
  uint32_t debouncedState;   // Final debounced state
  unsigned long debounceTimer[MAX_SWITCHES];
  bool debouncing[MAX_SWITCHES];
  
  // Port state for interrupt handling
  volatile uint8_t portState[3];    // PORTB, PORTD, PORTF
  volatile bool portChanged[3];
} hw;

// Default pin mapping - Teensy 2.0 layout
const uint8_t DEFAULT_PINS[MAX_SWITCHES] = {
  // Port D (pins 0-7) - 8 switches
  0, 1, 2, 3, 4, 5, 6, 7,
  // Port B (pins 8-15) - 8 switches (note: pin 13 is LED)
  8, 9, 10, 11, 12, 14, 15, 16,
  // Port F (pins 17-24) - 8 switches  
  17, 18, 19, 20, 21, 22, 23, 24
};

// Pin to port/bit conversion utility
void pinToPortBit(uint8_t pin, uint8_t* port, uint8_t* bit) {
  if (pin >= 8 && pin <= 15) {
    *port = 0; // PORTB
    *bit = pin - 8;
  } else if (pin >= 0 && pin <= 7) {
    *port = 1; // PORTD
    *bit = pin;
  } else if (pin >= 16 && pin <= 23) {
    *port = 2; // PORTF
    *bit = pin - 16;
  } else {
    *port = 255; // Invalid
    *bit = 255;
  }
}

// Interrupt Service Routines - capture port state changes
ISR(PCINT0_vect) { // PORTB changes
  hw.portState[0] = PINB;
  hw.portChanged[0] = true;
}

ISR(PCINT1_vect) { // PORTD changes
  hw.portState[1] = PIND;
  hw.portChanged[1] = true;
}

ISR(PCINT2_vect) { // PORTF changes
  hw.portState[2] = PINF;
  hw.portChanged[2] = true;
}

//==============================================================================
// PUBLIC FUNCTION IMPLEMENTATIONS
//==============================================================================

void setupSwitches() {
  // Configure switch pins
  hw.numSwitches = MAX_SWITCHES;
  
  for (int i = 0; i < MAX_SWITCHES; i++) {
    hw.switchPins[i] = DEFAULT_PINS[i];
    pinMode(hw.switchPins[i], INPUT_PULLUP);
    hw.debounceTimer[i] = 0;
    hw.debouncing[i] = false;
  }
  
  // Initialize state variables
  hw.currentState = 0;
  hw.debouncedState = 0;
  
  // Read initial port states
  hw.portState[0] = PINB;
  hw.portState[1] = PIND;
  hw.portState[2] = PINF;
  
  for (int i = 0; i < 3; i++) {
    hw.portChanged[i] = false;
  }
  
  // Enable Pin Change Interrupts for all three ports
  PCICR |= (1 << PCIE0) | (1 << PCIE1) | (1 << PCIE2);
  PCMSK0 = 0xFF; // Enable all PORTB pins
  PCMSK1 = 0xFF; // Enable all PORTD pins  
  PCMSK2 = 0xFF; // Enable all PORTF pins
  
  // Setup status LED
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);
  
  // Startup indication
  flashStatusLED(200);
}

uint32_t loopSwitches() {
  unsigned long currentTime = millis();
  
  // Process port changes from interrupts
  for (int port = 0; port < 3; port++) {
    if (hw.portChanged[port]) {
      hw.portChanged[port] = false;
      
      // Process each bit in the changed port
      uint8_t portBits = hw.portState[port];
      
      for (int bit = 0; bit < 8; bit++) {
        int switchIndex = port * 8 + bit;
        if (switchIndex >= MAX_SWITCHES) break;
        
        // Check if this switch changed (inverted due to pullup)
        bool pressed = !(portBits & (1 << bit));
        bool wasPressed = (hw.currentState >> switchIndex) & 1;
        
        if (pressed != wasPressed) {
          // State changed - start/restart debouncing
          hw.debounceTimer[switchIndex] = currentTime;
          hw.debouncing[switchIndex] = true;
          
          // Update raw current state immediately
          if (pressed) {
            hw.currentState |= (1UL << switchIndex);
          } else {
            hw.currentState &= ~(1UL << switchIndex);
          }
        }
      }
    }
  }
  
  // Process debouncing for all switches
  for (int i = 0; i < MAX_SWITCHES; i++) {
    if (hw.debouncing[i]) {
      if (currentTime - hw.debounceTimer[i] >= DEBOUNCE_MS) {
        // Debounce period completed
        hw.debouncing[i] = false;
        
        // Re-read actual pin state to confirm
        bool actualState = !digitalRead(hw.switchPins[i]);
        bool debouncedBit = (hw.debouncedState >> i) & 1;
        
        if (actualState != debouncedBit) {
          // State is stable and different - update debounced state
          if (actualState) {
            hw.debouncedState |= (1UL << i);
          } else {
            hw.debouncedState &= ~(1UL << i);
          }
        }
      }
    }
  }
  
  return hw.debouncedState;
}

void setStatusLED(bool state) {
  digitalWrite(LED_PIN, state ? HIGH : LOW);
}

void flashStatusLED(int duration_ms) {
  setStatusLED(true);
  delay(duration_ms);
  setStatusLED(false);
}

//==============================================================================
// OPTIONAL UTILITY FUNCTIONS
//==============================================================================

// Get number of configured switches
uint8_t getNumSwitches() {
  return hw.numSwitches;
}

// Get pin number for a switch index
uint8_t getSwitchPin(uint8_t switchIndex) {
  if (switchIndex < MAX_SWITCHES) {
    return hw.switchPins[switchIndex];
  }
  return 255; // Invalid
}

// Configure custom pin mapping (call before setupSwitches)
bool configureSwitchPins(const uint8_t* pins, uint8_t count) {
  if (count > MAX_SWITCHES) return false;
  
  hw.numSwitches = count;
  for (int i = 0; i < count; i++) {
    hw.switchPins[i] = pins[i];
  }
  
  return true;
}

// Debug function to print hardware status
void printHardwareStatus() {
  Serial.println(F("=== Hardware Status ==="));
  Serial.print(F("Configured switches: "));
  Serial.println(hw.numSwitches);
  
  Serial.print(F("Switch pins: "));
  for (int i = 0; i < hw.numSwitches; i++) {
    if (i > 0) Serial.print(F(", "));
    Serial.print(hw.switchPins[i]);
  }
  Serial.println();
  
  Serial.print(F("Port states: B=0x"));
  Serial.print(hw.portState[0], HEX);
  Serial.print(F(" D=0x"));
  Serial.print(hw.portState[1], HEX);
  Serial.print(F(" F=0x"));
  Serial.print(hw.portState[2], HEX);
  Serial.println();
  
  Serial.print(F("Debounced state: 0x"));
  Serial.println(hw.debouncedState, HEX);
  
  Serial.print(F("Active switches: "));
  bool anyActive = false;
  for (int i = 0; i < MAX_SWITCHES; i++) {
    if (hw.debouncedState & (1UL << i)) {
      if (anyActive) Serial.print(F(", "));
      Serial.print(i);
      anyActive = true;
    }
  }
  if (!anyActive) Serial.print(F("none"));
  Serial.println();
  
  Serial.print(F("Debouncing: "));
  bool anyDebouncing = false;
  for (int i = 0; i < MAX_SWITCHES; i++) {
    if (hw.debouncing[i]) {
      if (anyDebouncing) Serial.print(F(", "));
      Serial.print(i);
      anyDebouncing = true;
    }
  }
  if (!anyDebouncing) Serial.print(F("none"));
  Serial.println();
  
  Serial.println(F("======================"));
}

#endif // SWITCHES_TEENSY_H
