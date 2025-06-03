/*
 * switches-pico.h
 * 
 * Complete GPIO switch support for Raspberry Pi Pico (RP2040)
 * Uses shared RP2040SwitchesBase class
 * 
 * Pin mapping for RP2040:
 * - GPIO 0-28 available (GPIO 23, 24, 25 have special functions)
 * - GPIO 29 is ADC-only, not suitable for digital input
 * - We'll use GPIO 0-22 and 26-28 for maximum switch support (26 pins)
 */

#ifndef SWITCHES_PICO_H
#define SWITCHES_PICO_H

#include "hardware.kb2040/rp2040-switches.h"

#define MAX_SWITCHES 26  // Maximum realistic switch count for Pico

//==============================================================================
// PICO GPIO PIN MAPPING
//==============================================================================

// Available GPIO pins for switches on RP2040
// We exclude GPIO 23, 24, 25 as they're often used for:
// - GPIO 23: SMPS Power Save pin
// - GPIO 24: VBUS sense
// - GPIO 25: On-board LED
// - GPIO 29: ADC3 (ADC-only, no digital input)
//
// Available pins: 0-22, 26-28 = 26 total pins

const uint8_t PICO_GPIO_PINS[MAX_SWITCHES] = {
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
// PICO SWITCHES CLASS
//==============================================================================

class PicoSwitches : public RP2040SwitchesBase {
public:
    PicoSwitches(uint8_t num_pins = MAX_SWITCHES) 
        : RP2040SwitchesBase(num_pins, PICO_GPIO_PINS, MAX_SWITCHES) {}
    
    // Inherit all functionality from base class
    // No Pico-specific overrides needed
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
