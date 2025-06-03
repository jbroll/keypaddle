/*
 * switches-teensy.cpp
 * 
 * Implementation of 24-pin switch support for Teensy 2.0 (ATmega32U4)
 */

#include "switches.h"

//==============================================================================
// PIN MAPPING TABLE
//==============================================================================

// Complete pin mapping table for all 24 Teensy 2.0 pins
static const PinMapping pin_mappings[MAX_SWITCHES] PROGMEM = {
  // Arduino Pin 0-7 (Mixed ports)
  {&PORTB, &PINB, &DDRB, _BV(0), 0},   // Pin 0  = PB0 (SS)
  {&PORTB, &PINB, &DDRB, _BV(1), 1},   // Pin 1  = PB1 (SCLK)
  {&PORTB, &PINB, &DDRB, _BV(2), 2},   // Pin 2  = PB2 (MOSI)
  {&PORTB, &PINB, &DDRB, _BV(3), 3},   // Pin 3  = PB3 (MISO)
  {&PORTB, &PINB, &DDRB, _BV(7), 4},   // Pin 4  = PB7 (PWM)
  {&PORTD, &PIND, &DDRD, _BV(0), 5},   // Pin 5  = PD0 (SCL/PWM/INT0)
  {&PORTD, &PIND, &DDRD, _BV(1), 6},   // Pin 6  = PD1 (SDA/INT1)
  {&PORTD, &PIND, &DDRD, _BV(2), 7},   // Pin 7  = PD2 (RX/INT2)
  
  // Arduino Pin 8-15  
  {&PORTD, &PIND, &DDRD, _BV(3), 8},   // Pin 8  = PD3 (TX/INT3)
  {&PORTC, &PINC, &DDRC, _BV(6), 9},   // Pin 9  = PC6 (PWM)
  {&PORTC, &PINC, &DDRC, _BV(7), 10},  // Pin 10 = PC7 (PWM)
  {&PORTD, &PIND, &DDRD, _BV(6), 11},  // Pin 11 = PD6 (PWM/LED)
  {&PORTD, &PIND, &DDRD, _BV(7), 12},  // Pin 12 = PD7 (PWM)
  {&PORTB, &PINB, &DDRB, _BV(4), 13},  // Pin 13 = PB4 (PWM)
  {&PORTB, &PINB, &DDRB, _BV(5), 14},  // Pin 14 = PB5 (PWM)
  {&PORTB, &PINB, &DDRB, _BV(6), 15},  // Pin 15 = PB6 (PWM)
  
  // Arduino Pin 16-23 (Analog pins used as digital)
  {&PORTF, &PINF, &DDRF, _BV(7), 16},  // Pin 16 = PF7 (A0/ADC7)
  {&PORTF, &PINF, &DDRF, _BV(6), 17},  // Pin 17 = PF6 (A1/ADC6)
  {&PORTF, &PINF, &DDRF, _BV(5), 18},  // Pin 18 = PF5 (A2/ADC5)
  {&PORTF, &PINF, &DDRF, _BV(4), 19},  // Pin 19 = PF4 (A3/ADC4)
  {&PORTF, &PINF, &DDRF, _BV(1), 20},  // Pin 20 = PF1 (A4/ADC1)
  {&PORTF, &PINF, &DDRF, _BV(0), 21},  // Pin 21 = PF0 (A5/ADC0)
  {&PORTD, &PIND, &DDRD, _BV(4), 22},  // Pin 22 = PD4 (A6/ADC8)
  {&PORTD, &PIND, &DDRD, _BV(5), 23},  // Pin 23 = PD5 (A7)
};

//==============================================================================
// CLASS IMPLEMENTATION
//==============================================================================

TeensySwitches::TeensySwitches(uint8_t num_pins) : num_switches(min(num_pins, MAX_SWITCHES)) {
  switch_state = 0;
  debounced_state = 0;
  for (int i = 0; i < MAX_SWITCHES; i++) {
    last_change_time[i] = 0;
  }
}

void TeensySwitches::begin() {
  // Disable ADC to allow PORTF pins to work as digital I/O
  ADCSRA &= ~_BV(ADEN);
  
  // Configure each pin as input with pull-up
  for (uint8_t i = 0; i < num_switches; i++) {
    PinMapping mapping;
    memcpy_P(&mapping, &pin_mappings[i], sizeof(PinMapping));
    
    // Set as input (clear DDR bit)
    *mapping.ddr_reg &= ~mapping.bit_mask;
    
    // Enable pull-up (set PORT bit when pin is input)
    *mapping.port_reg |= mapping.bit_mask;
  }
  
  // Small delay for pull-ups to stabilize
  delay(10);
  
  // Initialize state
  readAllSwitches();
  debounced_state = switch_state;
}

uint32_t TeensySwitches::readAllSwitches() {
  uint32_t new_state = 0;
  
  // Read all ports once to minimize register access
  uint8_t portb_val = PINB;
  uint8_t portc_val = PINC; 
  uint8_t portd_val = PIND;
  uint8_t portf_val = PINF;
  
  // Process each switch and build state bitmap
  for (uint8_t i = 0; i < num_switches; i++) {
    PinMapping mapping;
    memcpy_P(&mapping, &pin_mappings[i], sizeof(PinMapping));
    
    bool pressed = false;
    
    // Select the appropriate cached port value
    if (mapping.pin_reg == &PINB) {
      pressed = !(portb_val & mapping.bit_mask);  // Inverted for pull-up
    } else if (mapping.pin_reg == &PINC) {
      pressed = !(portc_val & mapping.bit_mask);
    } else if (mapping.pin_reg == &PIND) {
      pressed = !(portd_val & mapping.bit_mask);
    } else if (mapping.pin_reg == &PINF) {
      pressed = !(portf_val & mapping.bit_mask);
    }
    
    if (pressed) {
      new_state |= (1UL << i);
    }
  }
  
  switch_state = new_state;
  return switch_state;
}

uint32_t TeensySwitches::update() {
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

//==============================================================================
// GLOBAL INSTANCE AND API
//==============================================================================

// Global instance
TeensySwitches switches;

void setupSwitches() {
  switches.begin();
}

uint32_t loopSwitches() {
  return switches.update();
}
