/*
 * switches-pico.cpp
 * 
 * Implementation of GPIO switch support for Raspberry Pi Pico (RP2040)
 * Supports up to 26 switches using available GPIO pins
 */

#include "switches.h"

//==============================================================================
// GPIO PIN MAPPING TABLE
//==============================================================================

// Available GPIO pins for switches on RP2040
// Ordered by preference - lower numbers first, avoiding special-purpose pins
static const uint8_t AVAILABLE_GPIO_PINS[MAX_SWITCHES] = {
  // GPIO 0-22 (23 pins) - general purpose I/O
  0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22,
  
  // GPIO 26-28 (3 pins) - additional general purpose I/O
  26, 27, 28
  
  // Excluded pins:
  // GPIO 23: SMPS Power Save pin (can be used but may affect power)
  // GPIO 24: VBUS sense (USB related)
  // GPIO 25: On-board LED (conflicts with LED)
  // GPIO 29: ADC3 only (no digital input capability)
};

//==============================================================================
// CLASS IMPLEMENTATION
//==============================================================================

PicoSwitches::PicoSwitches(uint8_t num_pins) : num_switches(min(num_pins, MAX_SWITCHES)) {
  switch_state = 0;
  debounced_state = 0;
  
  for (int i = 0; i < MAX_SWITCHES; i++) {
    last_change_time[i] = 0;
  }
  
  initializeGPIOPins();
}

void PicoSwitches::initializeGPIOPins() {
  // Map switch indices to actual GPIO pin numbers
  for (uint8_t i = 0; i < num_switches; i++) {
    gpio_pins[i] = AVAILABLE_GPIO_PINS[i];
  }
}

void PicoSwitches::begin() {
  // Configure each GPIO pin as input with pull-up
  for (uint8_t i = 0; i < num_switches; i++) {
    uint8_t pin = gpio_pins[i];
    
    // Use Arduino-style pin configuration for compatibility
    pinMode(pin, INPUT_PULLUP);
  }
  
  // Small delay for pull-ups to stabilize
  delay(10);
  
  // Initialize state
  readAllSwitches();
  debounced_state = switch_state;
}

uint32_t PicoSwitches::readAllSwitches() {
  uint32_t new_state = 0;
  
  // Read each configured GPIO pin
  for (uint8_t i = 0; i < num_switches; i++) {
    uint8_t pin = gpio_pins[i];
    
    // Read pin state - digitalRead returns HIGH when not pressed (pull-up)
    // We want bit set when pressed, so invert the reading
    bool pressed = (digitalRead(pin) == LOW);
    
    if (pressed) {
      new_state |= (1UL << i);
    }
  }
  
  switch_state = new_state;
  return switch_state;
}

uint32_t PicoSwitches::update() {
  uint32_t current_state = readAllSwitches();
  uint32_t changed = current_state ^ debounced_state;
  uint32_t now = millis();
  
  // Check each changed switch for debounce timeout
  for (uint8_t i = 0; i < num_switches; i++) {
    if (changed & (1UL << i)) {
      if (now - last_change_time[i] >= DEBOUNCE_MS) {
        // Update debounced state for this switch
        if (current_state & (1UL << i)) {
          debounced_state |= (1UL << i);   // Set bit
        } else {
          debounced_state &= ~(1UL << i);  // Clear bit  
        }
      }
      last_change_time[i] = now;
    }
  }
  
  return debounced_state;
}

uint8_t PicoSwitches::getGPIOPin(uint8_t switchIndex) const {
  if (switchIndex >= num_switches) {
    return 255; // Invalid pin number
  }
  return gpio_pins[switchIndex];
}

//==============================================================================
// GLOBAL INSTANCE AND API
//==============================================================================

// Global instance
PicoSwitches switches;

void setupSwitches() {
  switches.begin();
}

uint32_t loopSwitches() {
  return switches.update();
}