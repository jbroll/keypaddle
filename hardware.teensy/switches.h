/*
 * switches-teensy.h
 * 
 * Complete 24-pin switch support for Teensy 2.0 (ATmega32U4)
 * Supports all available digital I/O pins with efficient port-based scanning
 * 
 * Pin mapping based on official Teensy 2.0 pinout:
 * - Arduino pins 0-23 (24 total pins)
 * - Distributed across PORTB, PORTC, PORTD, PORTE, PORTF
 * - All pins support internal pull-ups for switches
 */

#ifndef SWITCHES_TEENSY_H
#define SWITCHES_TEENSY_H

#include <Arduino.h>

#include "config.h"

#define MAX_SWITCHES 24
#define DEBOUNCE_MS 50

// Pin to Port/Bit mapping based on Teensy 2.0 pinout
// Arduino Pin → Port.Bit mapping
struct PinMapping {
  volatile uint8_t* port_reg;    // PORT register for output/pullup
  volatile uint8_t* pin_reg;     // PIN register for input reading  
  volatile uint8_t* ddr_reg;     // DDR register for direction
  uint8_t bit_mask;              // Bit mask for this pin
  uint8_t arduino_pin;           // Arduino pin number
};

// Fast port reading using direct register access
class TeensySwitches {
private:
  uint32_t switch_state;
  uint32_t last_change_time[MAX_SWITCHES];
  uint32_t debounced_state;
  uint8_t num_switches;
  
public:
  TeensySwitches(uint8_t num_pins = MAX_SWITCHES);
  
  // Initialize all switch pins
  void begin();
  
  // Fast read of all switches using optimized port access
  uint32_t readAllSwitches();
  
  // Update with debouncing
  uint32_t update();
};

//==============================================================================
// EXPECTED API
//==============================================================================

// Global instance
extern TeensySwitches switches;

// API functions
void setupSwitches();
uint32_t loopSwitches();

#endif // SWITCHES_TEENSY_H
