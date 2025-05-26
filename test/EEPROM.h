/*
 * EEPROM.h Mock for Testing
 * Memory-backed EEPROM simulation for testing storage functions
 */

#ifndef EEPROM_H
#define EEPROM_H

#include <cstdint>
#include <cstring>
#include <cstdio>
#include <algorithm>

//==============================================================================
// EEPROM MOCK CONFIGURATION
//==============================================================================

#define EEPROM_SIZE 1024  // Simulate 1KB EEPROM (Teensy 2.0 has 512 bytes, but we'll use more for testing)

//==============================================================================
// EEPROM MOCK CLASS
//==============================================================================

class EEPROMClass {
private:
    uint8_t memory[EEPROM_SIZE];  // Simulated EEPROM memory
    
public:
    EEPROMClass() {
        // Initialize to 0xFF (typical EEPROM erased state)
        memset(memory, 0xFF, EEPROM_SIZE);
    }
    
    // Read a single byte
    uint8_t read(int address) {
        if (address < 0 || address >= EEPROM_SIZE) {
            return 0xFF;  // Return default value for out-of-bounds
        }
        return memory[address];
    }
    
    // Write a single byte
    void write(int address, uint8_t value) {
        if (address >= 0 && address < EEPROM_SIZE) {
            memory[address] = value;
        }
    }
    
    // Update a single byte (same as write for our mock)
    void update(int address, uint8_t value) {
        write(address, value);
    }
    
    // Get total EEPROM size
    int length() {
        return EEPROM_SIZE;
    }
    
    // Template functions for reading/writing multi-byte values
    template<typename T>
    T& get(int address, T& value) {
        if (address < 0 || address + sizeof(T) > EEPROM_SIZE) {
            // Return value unchanged for out-of-bounds
            return value;
        }
        
        uint8_t* ptr = reinterpret_cast<uint8_t*>(&value);
        for (size_t i = 0; i < sizeof(T); i++) {
            ptr[i] = memory[address + i];
        }
        return value;
    }
    
    template<typename T>
    const T& put(int address, const T& value) {
        if (address >= 0 && address + sizeof(T) <= EEPROM_SIZE) {
            const uint8_t* ptr = reinterpret_cast<const uint8_t*>(&value);
            for (size_t i = 0; i < sizeof(T); i++) {
                memory[address + i] = ptr[i];
            }
        }
        return value;
    }
    
    // Testing utilities
    void clear() {
        memset(memory, 0xFF, EEPROM_SIZE);
    }
    
    void fill(uint8_t value) {
        memset(memory, value, EEPROM_SIZE);
    }
    
    // Get raw memory pointer for testing inspection
    const uint8_t* getRawMemory() const {
        return memory;
    }
    
    // Copy memory state for comparison
    void copyMemory(uint8_t* dest) const {
        memcpy(dest, memory, EEPROM_SIZE);
    }
    
    // Compare with another memory state
    bool compareMemory(const uint8_t* other) const {
        return memcmp(memory, other, EEPROM_SIZE) == 0;
    }
    
    // Print memory contents for debugging (first 'count' bytes)
    void printMemory(int count = 64) const {
        printf("EEPROM Memory (first %d bytes):\n", std::min(count, EEPROM_SIZE));
        for (int i = 0; i < std::min(count, EEPROM_SIZE); i++) {
            if (i % 16 == 0) printf("%04X: ", i);
            printf("%02X ", memory[i]);
            if (i % 16 == 15) printf("\n");
        }
        if (count % 16 != 0) printf("\n");
    }
    
    // Check if memory range is all 0xFF (erased state)
    bool isErased(int start = 0, int length = -1) const {
        if (length == -1) length = EEPROM_SIZE - start;
        if (start < 0 || start + length > EEPROM_SIZE) return false;
        
        for (int i = start; i < start + length; i++) {
            if (memory[i] != 0xFF) return false;
        }
        return true;
    }
    
    // Count non-0xFF bytes in a range
    int countUsedBytes(int start = 0, int length = -1) const {
        if (length == -1) length = EEPROM_SIZE - start;
        if (start < 0 || start + length > EEPROM_SIZE) return 0;
        
        int count = 0;
        for (int i = start; i < start + length; i++) {
            if (memory[i] != 0xFF) count++;
        }
        return count;
    }
};

// Global EEPROM instance for Arduino compatibility
extern EEPROMClass EEPROM;

#endif // EEPROM_H