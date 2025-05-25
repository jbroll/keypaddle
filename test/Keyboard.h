/*
 * Keyboard.h Mock for Testing
 * Provides Teensy Keyboard library constants without actual USB functionality
 */

#ifndef KEYBOARD_H
#define KEYBOARD_H

// This file is included by map-parser-tables.cpp
// We only need the key constants, not the actual Keyboard functionality

// HID key codes that are referenced in the parser tables
// These match the Teensy Keyboard library constants

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

// Additional common keys
#define KEY_ENTER       0x0A
#define KEY_TAB         0x09
#define KEY_ESC         0x1B
#define KEY_BACKSPACE   0x08
#define KEY_SPACE       0x20

#endif // KEYBOARD_H