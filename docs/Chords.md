# Chording Keyboard System

## Overview

The chording system adds multi-key combination support to the UTF-8+ Key Paddle. It uses dynamic allocation for chord storage and integrates with the unified EEPROM storage system.

## Architecture

### Core Classes

```cpp
// Chord pattern storage (dynamic linked list)
struct ChordPattern {
  uint32_t keyMask;              // Bitmask of keys in chord
  char* macroSequence;           // UTF-8+ macro to execute
  ChordPattern* next;            // Linked list pointer
};

// Main chording engine
class ChordingEngine {
  // Dynamic storage
  ChordPattern* chordList;       // Linked list of chord patterns
  uint32_t modifierKeyMask;      // Which keys are modifiers
  
  // Runtime state
  ChordingState state;           // Current processing state
  uint32_t currentChord;         // Keys currently pressed
  // ... timing and change detection
};
```

### Processing Flow

```
1. Key Press Detected → Start chord building
2. More Keys Pressed → Update chord pattern  
3. Non-modifier Released → Match and execute chord
4. All Non-modifiers Released → Return to idle
```

### Storage Layout

```
EEPROM Layout:
[Switch Macros] → [MAGIC: 0x43484F52][ModifierMask][ChordCount][ChordData...][EndMarker: 0x00 0x00]
```

Chord data format:
- Magic: 0x43484F52 ("CHOR")
- Modifier mask: 32-bit bitmask of modifier keys
- Chord count: 32-bit count of chord patterns
- Chord entries: [32-bit keymask][null-terminated UTF-8+ string]
- End marker: Two null bytes (0x00 0x00)

## Command Interface

### Chord Management
```bash
# Add chord patterns
CHORD ADD 0,1 "hello"           # Keys 0+1 types "hello"
CHORD ADD 2+3+4 CTRL C          # Keys 2+3+4 sends Ctrl+C
CHORD ADD 1,5,7 +SHIFT "CAPS" -SHIFT  # Complex sequences

# Remove chords
CHORD REMOVE 0,1                # Remove specific chord
CHORD CLEAR                     # Remove all chords

# View chords
CHORD LIST                      # Show all defined chords
CHORD STATUS                    # Show current chording state
```

### Modifier Key Management
```bash
# Set modifier keys (integrated with CHORD command)
CHORD MODIFIERS 1,6             # Set keys 1&6 as modifiers
CHORD MODIFIERS                 # Show current modifier keys
CHORD MODIFIERS CLEAR           # Clear all modifiers
```

### Storage
```bash
SAVE                            # Save both switch macros and chords
LOAD                            # Load both switch macros and chords
```

## Usage Examples

### Basic Chording
```bash
# Define simple chord
CHORD ADD 0,1 "the"

# Usage: Press keys 0+1 simultaneously, release both → types "the"
```

### Stenography Style
```bash
# Set thumb as modifier
CHORD MODIFIERS 1

# Define lowercase/uppercase pairs
CHORD ADD 2 "a"                # Finger alone = lowercase
CHORD ADD 1,2 "A"               # Thumb + finger = uppercase

# Usage: 
# - Press/release key 2 → types "a"
# - Hold key 1, press/release key 2 → types "A" 
# - Key 1 can stay held for next letter
```

### Complex Patterns
```bash
# Programming shortcuts
CHORD ADD 0,1,2 "function"
CHORD ADD 3,4 "()"
CHORD ADD 5,6 "{}"

# System commands  
CHORD ADD 0,23 CTRL+ALT+SHIFT DEL    # Emergency task manager
CHORD ADD 1,22 ALT TAB               # Window switcher
```

## Storage Integration

The chording system uses the unified storage architecture:

1. `SAVE` command saves switch macros first, returns end offset
2. Chord storage begins at that offset using `saveChords()`
3. `LOAD` command loads switch macros first, returns end offset  
4. Chord loading begins at that offset using `loadChords()`

### Storage Interface Functions

```cpp
// Save chords starting at given offset, return new end offset
uint16_t saveChords(uint16_t startOffset, uint32_t modifierMask, 
                   void (*forEachChord)(void (*callback)(uint32_t, const char*)));

// Load chords from offset, return modifier mask (0 if no data)
uint32_t loadChords(uint16_t startOffset, 
                   bool (*addChord)(uint32_t, const char*),
                   void (*clearAllChords)());
```

## File Structure

### Core Files
- `chording.h/.cpp` - Chording engine implementation
- `chordStorage.h/.cpp` - EEPROM storage interface
- `commands/cmd-chord.cpp` - CHORD command implementation

### Integration
- `keypaddle.ino` - Main loop calls `processChording()`
- `serial-interface.cpp` - Command dispatch includes CHORD
- `storage.cpp` - Returns offsets for chord storage
- `commands/cmd-save.cpp` - Unified save operation
- `commands/cmd-load.cpp` - Unified load operation

## Example Configuration

```bash
# Set thumbs as modifiers
CHORD MODIFIERS 1,6             # Keys 1 and 6 are modifier keys

# Basic chord patterns
CHORD ADD 2 "a"                 # Single finger = lowercase
CHORD ADD 1,2 "A"               # Thumb + finger = uppercase  
CHORD ADD 2,3 "e"               # Two fingers = vowel
CHORD ADD 1,2,3 "E"             # Shift + two fingers = caps

# Punctuation with right thumb
CHORD ADD 6,2 "."               # Right thumb + finger = period
CHORD ADD 1,6,2 "!"             # Both thumbs + finger = exclamation

# Programming shortcuts
CHORD ADD 0,1,2 "function"      # Three-finger combination
CHORD ADD 3,4 "()"              # Parentheses
CHORD ADD 5,6 "{}"              # Braces

# System commands
CHORD ADD 0,23 CTRL+ALT+SHIFT DEL    # All corners = task manager

# Save configuration
SAVE                            # Saves both macros and chords
```