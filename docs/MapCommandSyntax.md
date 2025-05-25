# Complete MAP Command Syntax Specification

## Command Structure

```
MAP <key_index> [down|up] <macro_sequence>
```

- `<key_index>`: Integer 0-23 specifying which physical key
- `[down|up]`: Optional event specifier (defaults to `down` if omitted)
- `<macro_sequence>`: Space-separated sequence of tokens (see below)

## Token Types

### 1. Quoted Literal Strings
```
"text to type"
```
- Enclosed in double quotes
- Types the literal text exactly as written
- Supports escape sequences: `\r` `\n` `\t` `\a` `\e` `\"` `\\`

### 2. Modifier Operations

#### Atomic Operations (Default Behavior)
When modifiers appear without a prefix, they create atomic operations with the following token:

```
CTRL C              // Press CTRL+C, release both
ALT F4              // Press ALT+F4, release both
CTRL+SHIFT T        // Press CTRL+SHIFT+T, release all
CTRL+ALT+SHIFT DEL  // Press all modifiers+DEL, release all
```

**Rules for Atomic Operations:**
- Modifiers can be connected with `+`: `CTRL+SHIFT`, `ALT+CTRL`
- Must be followed by a key or keyword
- Automatically press all modifiers, execute key, then release all modifiers
- If no following token is provided, this is a parse error

#### Press and Hold Operations
```
+CTRL               // Press and hold CTRL
+CTRL+SHIFT         // Press and hold CTRL and SHIFT
+ALT+CTRL+SHIFT     // Press and hold all three modifiers
```

#### Release Operations
```
-CTRL               // Release CTRL
-CTRL+SHIFT         // Release CTRL and SHIFT
-ALT+CTRL+SHIFT     // Release all three modifiers
```

### 3. Function and Special Keys
```
F1 F2 F3 ... F12
ENTER TAB ESC BACKSPACE SPACE
UP DOWN LEFT RIGHT
HOME END PAGEUP PAGEDOWN
DELETE
```

## Modifier Behavior Rules

### Atomic Operations (Default)
- **`MOD key`**: Press modifiers + key, then release all
- **`MOD+MOD key`**: Press multiple modifiers + key, then release all
- **Error if no key follows**: `CTRL+SHIFT` alone is invalid

### Held Operations
- **`+MOD`**: Press and hold specified modifiers
- **`+MOD+MOD`**: Press and hold multiple modifiers  
- **Multiple `+MOD`**: Can chain multiple press operations

### Release Operations
- **`-MOD`**: Release specified modifiers
- **`-MOD+MOD`**: Release multiple modifiers
- **Used to end held modifier sequences**

## Examples

```bash
# Text and simple keys
MAP 1 "hello world"
MAP 2 F1
MAP 3 "Line 1\nLine 2\tTab\e"

# Atomic modifier combinations
MAP 4 CTRL C
MAP 5 CTRL+SHIFT T
MAP 6 ALT F4
MAP 7 SHIFT F10
MAP 8 CTRL+ALT+SHIFT DEL

# Held modifier sequences
MAP 9 +SHIFT "HELLO WORLD" -SHIFT
MAP 10 +CTRL+ALT "admin mode" -CTRL+ALT
MAP 11 +ALT TAB TAB TAB -ALT

# Gaming/modifier key simulation
MAP 12 down +SHIFT
MAP 12 up -SHIFT
MAP 13 down +CTRL+ALT
MAP 13 up -CTRL+ALT

# Automation sequences
MAP 14 CTRL L "github.com" ENTER
MAP 15 "Username: " TAB +SHIFT "password123" -SHIFT ENTER
MAP 16 +WIN R -WIN "notepad" ENTER

# Press/release events
MAP 17 down "Key pressed"
MAP 17 up "Key released"
MAP 18 down CTRL C
MAP 18 up CTRL V
MAP 19 down +SHIFT
MAP 19 up -SHIFT

# Temporary modifier states
MAP 20 +SHIFT "CAPS TEXT" -SHIFT " normal text"

# Multi-step workflows
MAP 21 +CTRL+SHIFT DEL -CTRL+SHIFT "Task Manager opened"
MAP 22 CTRL T "new tab" TAB "github.com" ENTER

# Gaming macros with held modifiers
MAP 23 +CTRL Q W E R -CTRL
MAP 24 down +ALT
MAP 24 up -ALT

# Text expansion with formatting
MAP 25 "Email: " "user@domain.com" TAB "Subject: " +SHIFT "URGENT" -SHIFT
```

## Modifier Behavior Summary

| Pattern | Example | Modifier Behavior | Key Behavior |
|---------|---------|-------------------|--------------|
| `MOD key` | `CTRL C` | Press+Release | Single key atomic |
| `MOD+MOD key` | `CTRL+SHIFT T` | Press+Release | Single key atomic |
| `+MOD` | `+CTRL` | Press+Hold | N/A |
| `+MOD+MOD` | `+CTRL+SHIFT` | Press+Hold | N/A |
| `-MOD` | `-CTRL` | Release | N/A |
| `-MOD+MOD` | `-CTRL+SHIFT` | Release | N/A |

## Supported Modifiers

- `CTRL` - Control key
- `ALT` - Alt key  
- `SHIFT` - Shift key
- `WIN`, `GUI`, `CMD` - Windows/GUI/Meta key (all aliases)

## Supported Special Keys

- **Function Keys:** `F1` through `F12`
- **Navigation:** `UP` `DOWN` `LEFT` `RIGHT` `HOME` `END` `PAGEUP` `PAGEDOWN`
- **Editing:** `ENTER` `TAB` `ESC` `BACKSPACE` `DELETE` `SPACE`

## Escape Sequences in Quoted Strings

| Sequence | Result |
|----------|--------|
| `\n` | Newline (Enter) |
| `\r` | Carriage Return |
| `\t` | Tab character |
| `\a` | Alert/Bell sound |
| `\e` | Escape key |
| `\"` | Literal quote character |
| `\\` | Literal backslash |

## Key Event Types

- `down` : Executes when key is pressed (optional, default)
- `up`: Executes when key is released

## Parse Errors

The following patterns will result in parse errors:

```bash
MAP 1 CTRL+SHIFT        # Error: No key follows modifier combination
MAP 2 +                 # Error: Empty modifier specification  
MAP 3 CTRL+UNKNOWN C    # Error: Unknown modifier name
MAP 4 CTRL UNKNOWN      # Error: Unknown key name
```