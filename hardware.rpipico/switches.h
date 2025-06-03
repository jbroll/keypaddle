/*
 * switches-pico.h
 * 
 * Complete GPIO switch support for Raspberry Pi Pico (RP2040)
 * Supports up to 26 switches using all available GPIO pins
 * 
 * Pin mapping for RP2040:
 * - GPIO 0-28 available (GPIO 23, 24, 25 have special functions)
 * - GPIO 29 is ADC-only, not suitable for digital input
 * - We'll use GPIO 0-22 and 26-28 for maximum switch support (26 pins)
 */

#ifndef SWITCHES_PICO_H
#define SWITCHES_PICO_H

#include <Arduino.h>
#include "config.h"

#define MAX_SWITCHES 26  // Maximum realistic switch count for Pico
#define DEBOUNCE_MS 50

//==============================================================================
// PICO GPIO CONFIGURATION
//==============================================================================

// GPIO pins available for switches on RP2040
// We exclude GPIO 23, 24, 25 as they're often used for:
// - GPIO 23: SMPS Power Save pin
// - GPIO 24: VBUS sense
// - GPIO 25: On-board LED
// - GPIO 29: ADC3 (ADC-only, no digital input)
//
// Available pins: 0-22, 26-28 = 26 total pins

struct PicoGPIOMapping {
  uint8_t gpio_num;      // GPIO number (0-28)
  bool available;        // Whether this GPIO is suitable for switches
};

//==============================================================================
// PICO SWITCHES CLASS
//==============================================================================

class PicoSwitches {
private:
  uint32_t switch_state;
  uint32_t last_change_time[MAX_SWITCHES];
  uint32_t debounced_state;
  uint8_t num_switches;
  uint8_t gpio_pins[MAX_SWITCHES];  // Actual GPIO pin numbers used
  
  void initializeGPIOPins();
  
public:
  PicoSwitches(uint8_t num_pins = MAX_SWITCHES);
  
  // Initialize all switch pins with pull-ups
  void begin();
  
  // Fast read of all switches 
  uint32_t readAllSwitches();
  
  // Update with debouncing
  uint32_t update();
  
  // Get the GPIO pin number for a given switch index
  uint8_t getGPIOPin(uint8_t switchIndex) const;
  
  // Get the number of configured switches
  uint8_t getNumSwitches() const { return num_switches; }
};

//==============================================================================
// GLOBAL API
//==============================================================================

// Global instance
extern PicoSwitches switches;

// API functions expected by main code
void setupSwitches();
uint32_t loopSwitches();

#endif // SWITCHES_PICO_H
