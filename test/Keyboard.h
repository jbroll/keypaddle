/*
 * Keyboard.h Mock for Testing
 * Provides Teensy Keyboard library constants and mock functionality for testing
 */

#ifndef KEYBOARD_H
#define KEYBOARD_H

#include <vector>
#include <string>
#include <cstdint>

//==============================================================================
// HID KEY CONSTANTS (using high end of uint8_t range to avoid ASCII conflicts)
//==============================================================================

// Use high end of uint8_t range (128-255) for special keys
#define KEY_F1          128
#define KEY_F2          129
#define KEY_F3          130
#define KEY_F4          131
#define KEY_F5          132
#define KEY_F6          133
#define KEY_F7          134
#define KEY_F8          135
#define KEY_F9          136
#define KEY_F10         137
#define KEY_F11         138
#define KEY_F12         139

#define KEY_UP_ARROW    140
#define KEY_DOWN_ARROW  141
#define KEY_LEFT_ARROW  142
#define KEY_RIGHT_ARROW 143

#define KEY_HOME        144
#define KEY_END         145
#define KEY_PAGE_UP     146
#define KEY_PAGE_DOWN   147
#define KEY_DELETE      148

// Modifier keys
#define KEY_LEFT_CTRL   150
#define KEY_LEFT_SHIFT  151
#define KEY_LEFT_ALT    152
#define KEY_LEFT_GUI    153

//==============================================================================
// ACTION ENCODING
//==============================================================================

// Action flags in high byte, HID code/character in low byte
#define ACTION_WRITE   0x0100
#define ACTION_PRESS   0x0200  
#define ACTION_RELEASE 0x0300

//==============================================================================
// MOCK KEYBOARD CLASS
//==============================================================================

class MockKeyboard {
private:
    std::vector<int> actions;
    
    std::string getKeyName(int code) const {
        // Handle special keys FIRST - before checking ASCII
        switch (code) {
            case KEY_LEFT_CTRL:  return "ctrl";
            case KEY_LEFT_SHIFT: return "shift";  
            case KEY_LEFT_ALT:   return "alt";
            case KEY_LEFT_GUI:   return "win";
            case KEY_F1:         return "f1";
            case KEY_F2:         return "f2";
            case KEY_F3:         return "f3";
            case KEY_F4:         return "f4";
            case KEY_F5:         return "f5";
            case KEY_F6:         return "f6";
            case KEY_F7:         return "f7";
            case KEY_F8:         return "f8";
            case KEY_F9:         return "f9";
            case KEY_F10:        return "f10";
            case KEY_F11:        return "f11";
            case KEY_F12:        return "f12";
            case KEY_UP_ARROW:   return "up";
            case KEY_DOWN_ARROW: return "down";
            case KEY_LEFT_ARROW: return "left";
            case KEY_RIGHT_ARROW: return "right";
            case KEY_HOME:       return "home";
            case KEY_END:        return "end";
            case KEY_PAGE_UP:    return "pageup";
            case KEY_PAGE_DOWN:  return "pagedown";
            case KEY_DELETE:     return "delete";
            
            // UTF-8+ key codes (sent directly by execution engine)
            case 0x13:           return "up";     // UTF8_KEY_UP
            case 0x14:           return "down";   // UTF8_KEY_DOWN  
            case 0x15:           return "left";   // UTF8_KEY_LEFT
            case 0x16:           return "right";  // UTF8_KEY_RIGHT
            case 0x17:           return "home";   // UTF8_KEY_HOME
            case 0x18:           return "end";    // UTF8_KEY_END
            case 0x19:           return "pageup"; // UTF8_KEY_PAGEUP
            case 0x1A:           return "pagedown"; // UTF8_KEY_PAGEDOWN
            case 0x1C:           return "delete"; // UTF8_KEY_DELETE
            
            case '\n':           return "\\n";
            case '\r':           return "\\r";
            case '\t':           return "\\t";
            case '\a':           return "\\a";
            case 0x1B:           return "\\e";  // Escape
            case 0x08:           return "\\b";  // Backspace
        }
        
        // Handle printable ASCII - preserve case (AFTER checking special keys)
        if (code >= 32 && code <= 126) {
            return std::string(1, (char)code);
        }
        
        return "?";  // Unknown key
    }
    
public:
    void write(uint8_t key) { 
        actions.push_back(ACTION_WRITE | key); 
    }
    
    void press(uint8_t key) { 
        actions.push_back(ACTION_PRESS | key); 
    }
    
    void release(uint8_t key) { 
        actions.push_back(ACTION_RELEASE | key); 
    }
    
    void clearActions() { 
        actions.clear(); 
    }
    
    std::vector<int> getActions() const { 
        return actions; 
    }
    
    std::string toString() const {
        std::string result;
        for (size_t i = 0; i < actions.size(); i++) {
            if (i > 0) result += " ";
            
            int action = actions[i] & 0xFF00;  // High byte
            int code = actions[i] & 0xFF;      // Low byte
            
            switch (action) {
                case ACTION_WRITE:   result += "write "; break;
                case ACTION_PRESS:   result += "press "; break;
                case ACTION_RELEASE: result += "release "; break;
                default:             result += "unknown "; break;
            }
            
            result += getKeyName(code);
        }
        return result;
    }
    
    // Arduino compatibility methods
    void begin() { /* no-op for testing */ }
};

// Global instance for Arduino compatibility
extern MockKeyboard Keyboard;

#endif // KEYBOARD_H