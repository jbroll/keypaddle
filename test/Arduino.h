/*
 * Minimal Arduino Mocking for Linux Testing - WITH CONTROLLABLE TIME
 * Provides just enough Arduino compatibility to test macro encode/decode
 */

#ifndef ARDUINO_H
#define ARDUINO_H

#include <chrono>
#include <cstdint>
#include <cstring>
#include <cstdlib>
#include <cctype>
#include <string>
#include <sstream>
#include <iomanip>

#include "Serial.h"
#include "EEPROM.h"
#include "Keyboard.h"

typedef const char* __FlashStringHelper;
#define F(string_literal) (string_literal)
#define PROGMEM
#define PGM_P const char*

// Arduino constants (must be defined before String class)
#define HEX 16
#define DEC 10

//==============================================================================
// CONTROLLABLE TIME FOR TESTING
//==============================================================================

class TestTimeControl {
private:
    static uint32_t currentTime;     // Declaration only
    static bool useControlledTime;   // Declaration only
    
public:
    // Set absolute time value
    static void setTime(uint32_t time) {
        currentTime = time;
        useControlledTime = true;
    }
    
    // Advance time by delta milliseconds
    static void advanceTime(uint32_t deltaMs) {
        currentTime += deltaMs;
        useControlledTime = true;
    }
    
    // Get current controlled time
    static uint32_t getTime() {
        return currentTime;
    }
    
    // Reset to real-time mode
    static void useRealTime() {
        useControlledTime = false;
    }
    
    // Check if we're in controlled time mode
    static bool isControlledTime() {
        return useControlledTime;
    }
    
    // Internal function called by millis()
    static uint32_t getCurrentTime() {
        if (useControlledTime) {
            return currentTime;
        } else {
            // Use real time
            using namespace std::chrono;
            static auto start_time = std::chrono::steady_clock::now();
            auto now = std::chrono::steady_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(now - start_time);
            return static_cast<uint32_t>(duration.count());
        }
    }
};

// Static member declarations (defined in Arduino.cpp)
// Note: These are declared here, defined in Arduino.cpp

// Controllable millis() function
inline uint32_t millis() {
    return TestTimeControl::getCurrentTime();
}

extern int __heap_start;
extern int* __brkval;

// Arduino String class with essential methods
class String {
private:
    std::string str;
    
public:
    String() = default;
    String(const char* s) : str(s ? s : "") {}
    String(const std::string& s) : str(s) {}
    String(char c) : str(1, c) {}
    String(int value, int base = 10) {
        if (base == 16) {
            std::stringstream ss;
            ss << std::hex << std::uppercase << value;
            str = ss.str();
        } else {
            str = std::to_string(value);
        }
    }
    
    String& operator+=(const char* s) { if (s) str += s; return *this; }
    String& operator+=(const String& s) { str += s.str; return *this; }
    String& operator+=(char c) { str += c; return *this; }
    String operator+(const char* s) const { String result(*this); result += s; return result; }
    String operator+(const String& s) const { String result(*this); result += s; return result; }
    
    const char* c_str() const { return str.c_str(); }
    size_t length() const { return str.length(); }
    
    // Conversion to std::string for test framework
    operator std::string() const { return str; }
};

inline void* memcpy_P(void* dest, const void* src, size_t n) {
    return memcpy(dest, src, n);
}

inline int strcasecmp(const char* s1, const char* s2) {
    while (*s1 && *s2) {
        char c1 = tolower(*s1);
        char c2 = tolower(*s2);
        if (c1 != c2) {
            return c1 - c2;
        }
        s1++;
        s2++;
    }
    return tolower(*s1) - tolower(*s2);
}

#endif // ARDUINO_H