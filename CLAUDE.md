# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

KeyPaddle is an Arduino-based programmable macro keyboard firmware with chording support. It runs on Teensy 2.0, RP2040 Pico, and KB2040 boards. The firmware presents as a USB HID keyboard and accepts serial commands for configuration.

## Build Commands

```bash
# Build for specific hardware (creates 'hardware' symlink and compiles)
make teensy    # Teensy 2.0
make pico      # RP2040 Pico
make kb2040    # Adafruit KB2040

# Generic build (uses current hardware symlink)
make build

# Upload to connected device
make upload

# Serial monitor
make mon

# Run all unit tests
make test

# Run specific test (from test/ directory)
cd test && make test-macros
cd test && make test-chord-states
```

Requires `arduino-cli` with appropriate board cores installed. See `Notes` file for core installation commands.

## Architecture

### Core Components

- **keypaddle.ino** - Main Arduino sketch, setup/loop, switch event dispatching
- **chording.h/cpp** - ChordingEngine class: state machine for chord detection with execution windows and modifier support
- **storage.h/cpp** - EEPROM persistence for switch macros (SwitchMacros struct)
- **chordStorage.h/cpp** - EEPROM persistence for chord patterns and modifier masks
- **macro-engine.h/cpp** - Executes UTF-8+ encoded macro sequences via Keyboard library
- **macro-encode.h/cpp** - Parses MAP command syntax into UTF-8+ byte encoding
- **macro-decode.h/cpp** - Decodes UTF-8+ bytes back to human-readable format
- **map-parser-tables.h/cpp** - Lookup tables for key names, modifiers, escape sequences
- **serial-interface.h/cpp** - Serial command processing, dispatches to command handlers

### Hardware Abstraction

Hardware-specific code lives in `hardware.*` directories (teensy, rpipico, kb2040). The `hardware` symlink points to the active target. Each contains:
- `CONFIG` - Build variables (FQBN, upload port)
- `switches.h/cpp` - Pin definitions and switch reading logic

### Command Handlers

Located in `commands/`:
- `cmd-map.cpp` - MAP command for individual key macros
- `cmd-chord.cpp` - CHORD ADD/REMOVE/LIST/CLEAR/SAVE/LOAD/STATUS
- `cmd-modifier.cpp` - Modifier key configuration
- `cmd-show.cpp`, `cmd-clear.cpp`, `cmd-help.cpp`, `cmd-load.cpp`, `cmd-save.cpp`, `cmd-stat.cpp`

### Key Data Flow

1. `loopSwitches()` reads physical switch state as bitmask
2. `processChording()` handles chord detection (state machine: IDLE -> BUILDING -> execution/cancellation)
3. If no chord handled, `processSwitchChanges()` triggers individual key macros
4. Macros execute via `executeUTF8Macro()` which sends HID keyboard events

### Macro Encoding

Macros use a UTF-8+ encoding scheme where:
- ASCII text is passed through
- Special keys (F1-F12, arrows, etc.) and modifiers are encoded as multi-byte sequences
- See `docs/MapCommandSyntax.md` for complete syntax reference

## Testing

Tests run natively on the host (not on hardware) using mock Arduino/Keyboard implementations in `test/Arduino.cpp`. Each test file compiles against the actual source files.

```bash
cd test
make test              # Run all tests
make test-chord-states # Run specific test
make clean             # Remove test binaries
```

## Configuration

- `config.h` defines `NUM_SWITCHES` (currently 9)
- Hardware CONFIG file sets `FQBN`, `PROG` (upload port), `PORT` (monitor port)
