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

//==============================================================================
// CONFIGURATION
//==============================================================================

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
// PORT-OPTIMIZED ACCESS FUNCTIONS
//==============================================================================

// Fast port reading using direct register access
class TeensySwitches {
private:
  uint32_t switch_state;
  uint32_t last_change_time[MAX_SWITCHES];
  uint32_t debounced_state;
  uint8_t num_switches;
  
public:
  TeensySwitches(uint8_t num_pins = MAX_SWITCHES) : num_switches(min(num_pins, MAX_SWITCHES)) {
    switch_state = 0;
    debounced_state = 0;
    for (int i = 0; i < MAX_SWITCHES; i++) {
      last_change_time[i] = 0;
    }
  }
  
  // Initialize all switch pins
  void begin() {
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
  
  // Fast read of all switches using optimized port access
  uint32_t readAllSwitches() {
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
  
  // Update with debouncing
  uint32_t update() {
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
  
  // Get current debounced state
  uint32_t getState() const {
    return debounced_state;
  }
  
  // Check if specific switch is pressed
  bool isPressed(uint8_t switch_num) const {
    if (switch_num >= num_switches) return false;
    return (debounced_state & (1UL << switch_num)) != 0;
  }
  
  // Get changes since last call
  uint32_t getChanges() {
    static uint32_t last_state = 0;
    uint32_t changes = debounced_state ^ last_state;
    last_state = debounced_state;
    return changes;
  }
  
  // Diagnostic functions
  void printPortStates() {
    Serial.print(F("PORTB: 0b"));
    Serial.print(PINB, BIN);
    Serial.print(F(" PORTC: 0b"));  
    Serial.print(PINC, BIN);
    Serial.print(F(" PORTD: 0b"));
    Serial.print(PIND, BIN);
    Serial.print(F(" PORTF: 0b"));
    Serial.println(PINF, BIN);
  }
  
  void printSwitchStates() {
    Serial.print(F("Switches (0-"));
    Serial.print(num_switches - 1);
    Serial.print(F("): "));
    
    for (int i = num_switches - 1; i >= 0; i--) {
      Serial.print(isPressed(i) ? '1' : '0');
      if (i % 8 == 0 && i > 0) Serial.print(' ');
    }
    Serial.println();
  }
  
  // Get Arduino pin number for switch index
  uint8_t getArduinoPin(uint8_t switch_num) const {
    if (switch_num >= num_switches) return 255;
    return pgm_read_byte(&pin_mappings[switch_num].arduino_pin);
  }
};

//==============================================================================
// INTERRUPT-BASED VERSION (Advanced)
//==============================================================================

// Pin change interrupt support for ultra-fast response
class TeensySwitchesInterrupt : public TeensySwitches {
private:
  volatile bool state_changed;
  
public:
  TeensySwitchesInterrupt(uint8_t num_pins = MAX_SWITCHES) : TeensySwitches(num_pins) {
    state_changed = false;
  }
  
  void begin() {
    TeensySwitches::begin();
    
    // Enable pin change interrupts for PORTB (pins 0-4, 13-15)
    PCICR |= _BV(PCIE0);   // Enable PORTB pin change interrupts
    PCMSK0 = 0xFF;         // Enable all PORTB pins
    
    // Note: PORTC, PORTD, PORTF don't have pin change interrupts on ATmega32U4
    // Would need to use external interrupts (INT0-INT6) for some pins
    // For full interrupt support, consider polling approach instead
  }
  
  bool hasChanged() {
    bool changed = state_changed;
    state_changed = false;
    return changed;
  }
  
  // Call this from PCINT0_vect interrupt
  void handlePinChangeInterrupt() {
    state_changed = true;
  }
};

//==============================================================================
// COMPATIBILITY FUNCTIONS
//==============================================================================

// Global instance for simple usage
extern TeensySwitches switches;

// Simple API functions
inline void initializeSwitches(uint8_t num_switches = MAX_SWITCHES) {
  switches = TeensySwitches(num_switches);
  switches.begin();
}

inline uint32_t readSwitches() {
  return switches.update();
}

inline bool isSwitchPressed(uint8_t switch_num) {
  return switches.isPressed(switch_num);
}

inline uint32_t getSwitchChanges() {
  return switches.getChanges();
}

//==============================================================================
// EXAMPLE USAGE
//==============================================================================

/*
// Basic usage:
#include "switches-teensy.h"

TeensySwitches switches(24);  // Use all 24 pins

void setup() {
  Serial.begin(115200);
  switches.begin();
  Serial.println("24-pin switch scanner ready");
}

void loop() {
  uint32_t state = switches.update();
  uint32_t changes = switches.getChanges();
  
  if (changes) {
    Serial.print("Switch changes: ");
    Serial.println(changes, BIN);
    switches.printSwitchStates();
  }
  
  delay(10);
}

// Pin mapping reference:
// Arduino Pin → Port.Pin → Function
// 0  → PB0 → SS           11 → PD6 → PWM/LED    
// 1  → PB1 → SCLK         12 → PD7 → PWM
// 2  → PB2 → MOSI         13 → PB4 → PWM        
// 3  → PB3 → MISO         14 → PB5 → PWM
// 4  → PB7 → PWM          15 → PB6 → PWM
// 5  → PD0 → SCL/PWM      16 → PF7 → A0
// 6  → PD1 → SDA          17 → PF6 → A1  
// 7  → PD2 → RX           18 → PF5 → A2
// 8  → PD3 → TX           19 → PF4 → A3
// 9  → PC6 → PWM          20 → PF1 → A4
// 10 → PC7 → PWM          21 → PF0 → A5
//                         22 → PD4 → A6
//                         23 → PD5 → A7
*/

#endif // SWITCHES_TEENSY_H