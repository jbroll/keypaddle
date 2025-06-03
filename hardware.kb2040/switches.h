/*
 * switches-kb2040.h
 * 
 * KB2040-specific GPIO switch support (RP2040-based)
 * Inherits from shared RP2040SwitchesBase class
 * 
 * KB2040 has fewer breakout pins than the full Pico:
 * - Available pins: 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 18, 19, 20, 26, 27, 28, 29
 * - Total: 18 pins available for switches
 */

#ifndef SWITCHES_KB2040_H
#define SWITCHES_KB2040_H

#include "hardware.kb2040/rp2040-switches.h"

#define MAX_SWITCHES 18  // KB2040 has 18 available GPIO pins

//==============================================================================
// KB2040 GPIO PIN MAPPING
//==============================================================================

// KB2040-specific GPIO pins available for switches
// Based on Adafruit KB2040 pinout - excludes non-broken-out pins
const uint8_t KB2040_GPIO_PINS[MAX_SWITCHES] = {
  // Main GPIO pins (0-10)
  0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10,
  
  // Additional GPIO pins available on KB2040
  18, 19, 20,  // SPI pins (can be used as GPIO)
  26, 27, 28, 29  // ADC pins (can be used as digital GPIO)
  
  // Excluded pins:
  // GPIO 11-17: Not broken out on KB2040
  // GPIO 21-25: Not broken out on KB2040 (including on-board LED at 25)
};

//==============================================================================
// KB2040 SWITCHES CLASS
//==============================================================================

class KB2040Switches : public RP2040SwitchesBase {
public:
    KB2040Switches(uint8_t num_pins = MAX_SWITCHES) 
        : RP2040SwitchesBase(num_pins, KB2040_GPIO_PINS, MAX_SWITCHES) {}
    
    // Inherit all functionality from base class
    // No KB2040-specific overrides needed
};

//==============================================================================
// GLOBAL API
//==============================================================================

// Global instance
extern KB2040Switches switches;

// API functions expected by main code
void setupSwitches();
uint32_t loopSwitches();

#endif // SWITCHES_KB2040_H
