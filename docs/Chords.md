# Chording Keyboard System Summary

## Overview

The chording system adds multi-key combination support to the UTF-8+ Key Paddle, enabling stenography-style input and complex key combinations. It integrates seamlessly with the existing single-key macro system and provides full EEPROM persistence.

## Key Features

### 1. **Flexible Chord Recognition**
- **Press-and-release interface**: Standard chording behavior
- **Modifier key support**: Designated keys (like thumbs) don't need to be released
- **Timeout protection**: 50ms window for human finger coordination
- **State machine**: Clean handling of chord building, matching, and passthrough

### 2. **Dynamic Storage**
- **Linked list**: No arbitrary limits on number of chords
- **Dynamic allocation**: Memory only used for defined chords
- **EEPROM persistence**: Chords survive reboots
- **Integrated storage**: Works alongside existing switch macro storage

### 3. **Stenography Support**
- **Modifier keys**: Thumb keys can be held while typing letter combinations
- **Immediate triggering**: Chords execute when non-modifier keys are released
- **Natural flow**: Hold shift, tap letters, shift stays held for next chord

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
[0x0000] Switch Macro Magic + Data
[CHORD_START] Chord Magic (0xCH0RD)
[+4] Modifier Key Mask (32-bit)
[+8] Chord Count (16-bit) 
[+10] Chord Data: [KeyMask][MacroLen][MacroData]...
```

## Command Interface

### Chord Management
```bash
# Define chord patterns
CHORD ADD 0,1 "hello"           # Keys 0+1 types "hello"
CHORD ADD 2+3+4 CTRL C          # Keys 2+3+4 sends Ctrl+C
CHORD ADD 1,5,7 +SHIFT "CAPS" -SHIFT  # Complex sequences

# Remove chords
CHORD REMOVE 0,1                # Remove specific chord
CHORD CLEAR                     # Remove all chords

# View chords
CHORD LIST                      # Show all defined chords
CHORD STATUS                    # Show current chording state

# Persistence
CHORD SAVE                      # Save to EEPROM
CHORD LOAD                      # Load from EEPROM
```

### Modifier Key Management
```bash
# Set modifier keys (thumbs, etc.)
MODIFIER SET 1                  # Key 1 becomes modifier
MODIFIER SET 6                  # Key 6 becomes modifier
MODIFIER UNSET 1                # Remove modifier designation
MODIFIER LIST                   # Show all modifier keys
MODIFIER CLEAR                  # Clear all modifier designations
```

### System Commands
```bash
HELP                           # Show all commands
STAT                           # Show system status
SAVE/LOAD                      # Switch macro persistence (separate)
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
MODIFIER SET 1

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

## Integration with Existing System

### Compatibility
- **Non-destructive**: Individual key macros still work when no chord matches
- **Layered processing**: Chording checks first, falls back to individual keys
- **Same macro engine**: Chords use identical UTF-8+ macro system
- **Independent storage**: Chord and switch macro persistence are separate

### Processing Priority
```
1. Hardware switch scanning (unchanged)
2. Chord pattern matching (new)
3. Individual key macro execution (existing)
4. Serial command processing (unchanged)
```

### Memory Usage
- **Efficient**: Only allocates memory for defined chords
- **Scalable**: No hard limits on chord count
- **Clean**: Automatic cleanup on system reset

## State Machine

```
CHORD_IDLE:
  Key press → CHORD_BUILDING

CHORD_BUILDING:
  More keys → Update pattern, reset timeout
  Non-modifier release → Try match, execute if found
  Timeout → Evaluate current pattern

CHORD_MATCHED:
  All non-modifiers released → CHORD_IDLE
  (Suppress individual key processing)

CHORD_PASSTHROUGH:
  All non-modifiers released → CHORD_IDLE
  (Allow individual key processing)
```

## File Structure

### Core Files
- `chording.h` - Interface and class definitions
- `chording.cpp` - Implementation with storage integration
- `commands/cmd-chord.cpp` - CHORD command implementation
- `commands/cmd-modifier.cpp` - MODIFIER command implementation

### Integration Points
- `keypaddle.ino` - Main loop integration
- `serial-interface.cpp` - Command dispatch
- `storage.cpp` - EEPROM layout coordination

## Benefits

1. **True stenography support**: Modifier keys that don't need release
2. **Unlimited chord patterns**: Dynamic allocation removes arbitrary limits
3. **Persistent configuration**: Chords survive reboots via EEPROM
4. **Backward compatible**: Existing macros continue to work unchanged
5. **Memory efficient**: Only uses memory for defined chords
6. **Fast execution**: Efficient bit operations for chord matching
7. **Flexible syntax**: Same UTF-8+ macro language for everything

## Example Stenography Setup

```bash
# Set thumbs as modifiers
MODIFIER SET 1    # Left thumb
MODIFIER SET 6    # Right thumb

# Letters (fingers 2,3,4,7,8,9)
CHORD ADD 2 "a"              # Index finger
CHORD ADD 1,2 "A"            # Shift + index
CHORD ADD 2,3 "e"            # Two fingers
CHORD ADD 1,2,3 "E"          # Shift + two fingers

# Punctuation
CHORD ADD 6,2 "."            # Right thumb + finger
CHORD ADD 1,6,2 "!"          # Both thumbs + finger

# Save configuration
CHORD SAVE
MODIFIER SAVE  # (Actually saved with CHORD SAVE)
```

This creates a powerful, flexible chording system that maintains the simplicity and reliability of the original single-key macro system while adding advanced multi-key capabilities.