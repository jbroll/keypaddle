/*
 * Serial.h Mock for Testing
 * Provides Arduino Serial library mock functionality for testing serial interface
 */

#ifndef SERIAL_H
#define SERIAL_H

#include <vector>
#include <string>
#include <sstream>
#include <iostream>
#include <cstdint>

// Forward declaration of Arduino types
class String;
typedef const char* __FlashStringHelper;

//==============================================================================
// MOCK SERIAL CLASS
//==============================================================================

class MockSerial {
private:
    std::vector<std::string> outputLines;
    std::string currentLine;
    std::string inputBuffer;
    size_t inputPosition;
    bool echoEnabled;
    
public:
    MockSerial() : inputPosition(0), echoEnabled(true) {
        clear();
    }
    
    // Arduino Serial compatibility methods
    void begin(unsigned long baud) {
        // Mock implementation - just clear state
        clear();
    }
    
    bool available() {
        return inputPosition < inputBuffer.length();
    }
    
    char read() {
        if (inputPosition < inputBuffer.length()) {
            return inputBuffer[inputPosition++];
        }
        return -1; // No data available
    }
    
    void print(const char* str) {
        if (str) currentLine += str;
    }
    
    void print(const std::string& str) {
        currentLine += str;
    }
    
    void print(char c) {
        currentLine += c;
    }
    
    void print(int value) {
        currentLine += std::to_string(value);
    }
    
    void print(int value, int base) {
        if (base == 16) {
            std::stringstream ss;
            ss << std::hex << std::uppercase << value;
            currentLine += ss.str();
        } else {
            currentLine += std::to_string(value);
        }
    }
    
    void println() {
        outputLines.push_back(currentLine);
        currentLine.clear();
    }
    
    void println(const char* str) {
        if (str) currentLine += str;
        println();
    }
    
    void println(const std::string& str) {
        currentLine += str;
        println();
    }
    
    void println(char c) {
        currentLine += c;
        println();
    }
    
    void println(int value) {
        currentLine += std::to_string(value);
        println();
    }
    
    void println(int value, int base) {
        print(value, base);
        println();
    }
    
    // Support for F() macro strings (treat as regular strings in test)
    void print(const __FlashStringHelper* str) {
        print((const char*)str);
    }
    
    void println(const __FlashStringHelper* str) {
        println((const char*)str);
    }
    
    // Testing utility methods
    void clear() {
        outputLines.clear();
        currentLine.clear();
        inputBuffer.clear();
        inputPosition = 0;
    }
    
    void setInput(const std::string& input) {
        inputBuffer = input;
        inputPosition = 0;
    }
    
    void appendInput(const std::string& input) {
        inputBuffer += input;
    }
    
    std::vector<std::string> getOutputLines() const {
        // Include current line if it has content
        std::vector<std::string> result = outputLines;
        if (!currentLine.empty()) {
            result.push_back(currentLine);
        }
        return result;
    }
    
    std::string getFullOutput() const {
        std::string result;
        for (size_t i = 0; i < outputLines.size(); i++) {
            if (i > 0) result += "\n";
            result += outputLines[i];
        }
        if (!currentLine.empty()) {
            if (!outputLines.empty()) result += "\n";
            result += currentLine;
        }
        return result;
    }
    
    std::string getLastLine() const {
        if (!currentLine.empty()) {
            return currentLine;
        }
        if (!outputLines.empty()) {
            return outputLines.back();
        }
        return "";
    }
    
    size_t getLineCount() const {
        size_t count = outputLines.size();
        if (!currentLine.empty()) count++;
        return count;
    }
    
    bool hasOutput() const {
        return !outputLines.empty() || !currentLine.empty();
    }
    
    void setEcho(bool enabled) {
        echoEnabled = enabled;
    }
    
    // Search for specific text in output
    bool containsOutput(const std::string& searchText) const {
        std::string fullOutput = getFullOutput();
        return fullOutput.find(searchText) != std::string::npos;
    }
    
    bool containsLine(const std::string& searchText) const {
        auto lines = getOutputLines();
        for (const auto& line : lines) {
            if (line.find(searchText) != std::string::npos) {
                return true;
            }
        }
        return false;
    }
    
    // Count occurrences of text in output
    int countOccurrences(const std::string& searchText) const {
        std::string fullOutput = getFullOutput();
        int count = 0;
        size_t pos = 0;
        while ((pos = fullOutput.find(searchText, pos)) != std::string::npos) {
            count++;
            pos += searchText.length();
        }
        return count;
    }
    
    // Debug output for test diagnostics
    void printDebugOutput() const {
        std::cout << "=== Serial Output Debug ===" << std::endl;
        auto lines = getOutputLines();
        for (size_t i = 0; i < lines.size(); i++) {
            std::cout << "Line " << i << ": '" << lines[i] << "'" << std::endl;
        }
        std::cout << "Total lines: " << lines.size() << std::endl;
        std::cout << "==========================" << std::endl;
    }
    
    // Simulate readline behavior for testing
    std::string simulateReadline(const std::string& input) {
        setInput(input + "\n");
        
        // This would normally be called by the readline function
        // For testing, we'll simulate the key parts
        std::string result;
        while (available()) {
            char c = read();
            if (c == '\n' || c == '\r') {
                if (!result.empty()) {
                    return result;
                }
            } else if (c >= 32 && c <= 126) {  // Printable chars
                result += c;
                if (echoEnabled) {
                    print(c);  // Echo back
                }
            }
        }
        return result;
    }
};

// Global instance for Arduino compatibility
extern MockSerial Serial;

#endif // SERIAL_H