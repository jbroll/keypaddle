#pragma once
#include <string>
#include <cstring>
#include <cstdlib>
#include <cctype>

// Core Arduino replacements
using String = std::string;
#define F(x) x
#define PROGMEM

// Minimal function mocks
inline unsigned long millis() { return 0; }
inline void delay(int) {}

// Serial mock for any debug output
struct MockSerial {
    template<typename T> void print(T) {}
    template<typename T> void println(T) {}
} Serial;