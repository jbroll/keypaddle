/*
 * Minimal Arduino Mocking for Linux Testing
 * Provides just enough Arduino compatibility to test macro encode/decode
 */

#ifndef ARDUINO_H
#define ARDUINO_H

#include <cstdint>
#include <cstring>
#include <cstdlib>
#include <cctype>
#include <string>
#include <sstream>
#include <iomanip>

//==============================================================================
// ARDUINO TYPES AND CONSTANTS
//==============================================================================

typedef const char* __FlashStringHelper;
#define F(string_literal) (string_literal)
#define PROGMEM
#define PGM_P const char*

// Arduino constants (must be defined before String class)
#define HEX 16
#define DEC 10

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

// Arduino constants
#define HEX 16
#define DEC 10

//==============================================================================
// PROGMEM UTILITIES
//==============================================================================

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

//==============================================================================
// TEENSY KEYBOARD CONSTANTS
//==============================================================================

// HID key codes for special keys
#define KEY_F1          0x3A
#define KEY_F2          0x3B
#define KEY_F3          0x3C
#define KEY_F4          0x3D
#define KEY_F5          0x3E
#define KEY_F6          0x3F
#define KEY_F7          0x40
#define KEY_F8          0x41
#define KEY_F9          0x42
#define KEY_F10         0x43
#define KEY_F11         0x44
#define KEY_F12         0x45

#define KEY_UP_ARROW    0x52
#define KEY_DOWN_ARROW  0x51
#define KEY_LEFT_ARROW  0x50
#define KEY_RIGHT_ARROW 0x4F

#define KEY_HOME        0x4A
#define KEY_END         0x4D
#define KEY_PAGE_UP     0x4B
#define KEY_PAGE_DOWN   0x4E
#define KEY_DELETE      0x4C

#endif // ARDUINO_H