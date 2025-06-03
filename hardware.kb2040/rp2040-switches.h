/*
 * rp2040-switches-base.h
 * 
 * Shared base class for RP2040-based switch implementations
 * Provides common functionality for RPi Pico, KB2040, and other RP2040 boards
 */

#ifndef RP2040_SWITCHES_BASE_H
#define RP2040_SWITCHES_BASE_H

#include <Arduino.h>
#include "../config.h"

#define DEBOUNCE_MS 50

//==============================================================================
// SHARED RP2040 BASE CLASS
//==============================================================================

class RP2040SwitchesBase {
protected:
    uint32_t switch_state;
    uint32_t last_change_time[32];  // Max possible switches for any RP2040 board
    uint32_t debounced_state;
    uint8_t num_switches;
    uint8_t max_available_switches;
    const uint8_t* available_pins;  // Pointer to board-specific pin array
    uint8_t gpio_pins[32];  // Actual GPIO pin numbers used
    
    void initializeGPIOPins() {
        // Map switch indices to actual GPIO pin numbers from board-specific array
        for (uint8_t i = 0; i < num_switches; i++) {
            gpio_pins[i] = available_pins[i];
        }
    }
    
public:
    RP2040SwitchesBase(uint8_t num_pins, const uint8_t* pin_array, uint8_t max_pins) 
        : num_switches(min(num_pins, max_pins)), 
          max_available_switches(max_pins),
          available_pins(pin_array) {
        
        switch_state = 0;
        debounced_state = 0;
        
        for (int i = 0; i < 32; i++) {
            last_change_time[i] = 0;
        }
        
        initializeGPIOPins();
    }
    
    // Initialize all switch pins with pull-ups
    void begin() {
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
    
    // Fast read of all switches 
    uint32_t readAllSwitches() {
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
    
    // Get the GPIO pin number for a given switch index
    uint8_t getGPIOPin(uint8_t switchIndex) const {
        if (switchIndex >= num_switches) {
            return 255; // Invalid pin number
        }
        return gpio_pins[switchIndex];
    }
    
    // Get the number of configured switches
    uint8_t getNumSwitches() const { 
        return num_switches; 
    }
    
    // Get the maximum available switches for this board
    uint8_t getMaxAvailableSwitches() const {
        return max_available_switches;
    }
};

#endif // RP2040_SWITCHES_BASE_H
