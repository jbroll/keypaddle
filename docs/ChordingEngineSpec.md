# Chord Engine Specification

## Overview

The chord engine processes multi-key combinations (chords) with conflict prevention, execution windows, and cancellation mechanisms. Individual switch macros and chord patterns are mutually exclusive - a switch cannot be assigned to both types.

## State Machine

### States
- **IDLE**: No keys pressed, normal operation
- **CHORD_BUILDING**: Chord-capable keys pressed, individual key processing suppressed
- **CANCELLATION**: Non-chord key pressed during building, all processing suppressed

### State Transitions
```
IDLE → CHORD_BUILDING: Any chord-capable key pressed
CHORD_BUILDING → CANCELLATION: Non-chord, non-modifier key pressed
CHORD_BUILDING → IDLE: Chord executed or all keys released
CANCELLATION → IDLE: All keys released within execution window
CANCELLATION → CHORD_BUILDING: 2-second timeout with keys still pressed
```

## Key Processing Rules

### Individual Key Suppression
- Individual key macros are suppressed in CHORD_BUILDING and CANCELLATION states
- Suppression applies while any chord-capable key is physically pressed
- Individual keys resume normal operation only in IDLE state

### Chord Pattern Building
- `capturedChord` accumulates all chord-capable keys pressed during the gesture
- Pattern includes modifier keys for lookup purposes
- Non-chord keys do not contribute to pattern

### Conflict Prevention
- Commands reject switch assignments that conflict with existing assignments
- `MAP` command fails if switch is used in any chord pattern
- `CHORD ADD` command fails if any switch has individual macros

## Timing Mechanisms

### Execution Window
- Default duration: 50ms (private variable, configurable via setter)
- Triggered on first key release in CHORD_BUILDING or CANCELLATION states
- Behavior on window expiration:
  - All keys released + CHORD_BUILDING state → Execute chord, transition to IDLE
  - All keys released + CANCELLATION state → Transition to IDLE (no execution)
  - Some keys held → Update `capturedChord` to currently pressed keys, reset window

### Cancellation Window
- Duration: 2000ms
- Triggered when non-chord, non-modifier key pressed during CHORD_BUILDING
- Timer resets on each additional non-chord key press
- Window expiration: Return to CHORD_BUILDING state with currently pressed chord keys

## Data Structures

### Core State
```cpp
enum ChordState { IDLE, CHORD_BUILDING, CANCELLATION };
ChordState state;
uint32_t capturedChord;              // Accumulated chord pattern
uint32_t chordSwitchesMask;          // Bitmask of all switches used in chords
uint32_t modifierKeyMask;            // Bitmask of modifier switches
```

### Timing State
```cpp
uint32_t executionWindowMs = 50;     // Execution window duration
uint32_t executionWindowStart;      // Window start time
bool executionWindowActive;          // Window status
uint32_t cancellationStartTime;     // Cancellation window start time
uint32_t pressedKeys;                // Currently pressed switches
uint32_t lastSwitchState;            // Previous switch state for change detection
```

### Chord Storage
```cpp
struct ChordPattern {
    uint32_t keyMask;                // Switch combination bitmask
    char* macroSequence;             // UTF-8+ macro string (malloc'd)
    ChordPattern* next;              // Linked list pointer
};
ChordPattern* chordList;             // Dynamic chord storage
```

## Processing Algorithm

### Main Processing Loop
```cpp
bool processChording(uint32_t currentSwitchState) {
    uint32_t chordSwitches = currentSwitchState & chordSwitchesMask;
    uint32_t pressed = chordSwitches & ~(lastSwitchState & chordSwitchesMask);
    uint32_t released = (lastSwitchState & chordSwitchesMask) & ~chordSwitches;
    
    // State-specific processing
    // Execution window management
    // Pattern building and lookup
    // Return true if individual key processing should be suppressed
}
```

### Switch Filtering
- Input switch state filtered by `chordSwitchesMask` to isolate chord-capable switches
- Non-chord switches trigger cancellation but do not contribute to patterns
- Modifier switches included in patterns but excluded from release detection

### Pattern Execution
- Chord lookup occurs only when all non-modifier chord keys are released
- Uses `capturedChord` for pattern matching
- Executes macro sequence via `executeUTF8Macro()` if pattern found
- No execution occurs in CANCELLATION state

## Configuration Management

### Chord Switches Mask
- Automatically maintained by `updateChordSwitchesMask()`
- Updated on chord add/remove/clear operations
- Provides O(1) conflict detection via `isSwitchUsedInChords()`

### Modifier Keys
- Independent of chord patterns
- Managed via `setModifierKey()`, `clearAllModifiers()`
- Affect release detection logic but not pattern filtering

## Integration Points

### Command Interface
- `CHORD ADD/REMOVE/LIST/CLEAR` commands
- Conflict detection during MAP and CHORD operations
- Status reporting via query functions

### Storage System
- Persistent storage via chord storage interface
- Automatic mask rebuilding on load operations
- Integrated with unified EEPROM layout

### Main Loop Integration
- Returns boolean indicating individual key suppression
- Processes switch state changes
- Coordinates with individual key processing system