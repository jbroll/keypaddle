# KeyPaddle

Arduino firmware for programmable macro keyboards with chording support. Presents as USB HID keyboard, configured via serial commands.

## Supported Hardware

- Teensy 2.0
- RP2040 Pico
- Adafruit KB2040

## Building

Requires `arduino-cli` with board cores installed:

```bash
# Install cores
arduino-cli core install teensy:avr          # Teensy
arduino-cli core install rp2040:rp2040       # Pico/KB2040

# Build
make teensy   # or: make pico, make kb2040

# Upload
make upload

# Serial monitor (115200 baud)
make mon
```

## Serial Commands

### Individual Key Macros

```
MAP <key> [down|up] <macro>   Set macro for key press/release
SHOW <key|ALL> [up]           Show configured macros
CLEAR <key> [up]              Clear macro
SAVE                          Save to EEPROM
LOAD                          Load from EEPROM
```

### Chording

```
CHORD ADD <keys> <macro>      Add chord (keys: 0,1,2 or 0+1+2)
CHORD REMOVE <keys>           Remove chord
CHORD LIST                    List all chords
CHORD CLEAR                   Clear all chords
CHORD MODIFIERS [keys]        Set/show modifier keys
CHORD STATUS                  Show chording state
```

### Macro Syntax

```
"text"                        Literal text (supports \n \t \e \\ \")
CTRL C                        Modifier + key (atomic press/release)
CTRL+SHIFT T                  Multiple modifiers + key
+SHIFT ... -SHIFT             Hold/release modifiers
F1-F12 ENTER TAB ESC          Special keys
UP DOWN LEFT RIGHT            Arrow keys
HOME END PAGEUP PAGEDOWN      Navigation
```

## Examples

```
MAP 0 "hello world"
MAP 1 CTRL C
MAP 2 down +SHIFT
MAP 2 up -SHIFT
MAP 3 CTRL+ALT DEL

CHORD ADD 0,1 "the"
CHORD ADD 2,3,4 CTRL+SHIFT T
CHORD MODIFIERS 0             # Key 0 as modifier
SAVE
```

## Testing

```bash
make test                     # Run all tests
cd test && make test-macros   # Run specific test
```

## License

MIT
