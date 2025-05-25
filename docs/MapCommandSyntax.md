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
- Supports escape sequences: `\r` `\n` `\t` `\a` `\"` `\\`

### 2. Modifier Chains and Control Syntax

The MAP command supports multiple syntax patterns for modifier keys, providing explicit control over when modifiers are pressed, held, or released.

#### Pattern 1: Traditional Atomic Combinations
```
CTRL-C        // Explicit dash: press Ctrl+C, release both
ALT-F4        // Press ALT+F4, release both
CTRL-SHIFT-T  // Press all three, release all
```

#### Pattern 2: Space-Separated Atomic Operations
```
CTRL TAB      // Press CTRL+TAB atomically, release both
SHIFT F1      // Press SHIFT+F1 atomically, release both
ALT ENTER     // Press ALT+ENTER atomically, release both
```

#### Pattern 3: Press and Hold Modifiers
```
+CTRL         // Press and hold CTRL
+CTRL+SHIFT   // Press and hold CTRL and SHIFT
+ALT+CTRL+SHIFT // Press and hold all three modifiers
```

#### Pattern 4: Release Modifiers
```
-CTRL         // Release CTRL
-CTRL+SHIFT   // Release CTRL and SHIFT
-ALT+CTRL+SHIFT // Release all three modifiers
```

### 3. Function and Special Keys
```
F1 F2 F3 ... F12
ENTER TAB ESC BACKSPACE
UP DOWN LEFT RIGHT
HOME END PAGEUP PAGEDOWN
DELETE SPACE
```

## Modifier Behavior Rules

### Atomic Operations (Default)
- **`MOD-key`**: Press modifiers + key, then release all
- **`MOD key`**: Press modifiers + next token, then release all

### Held Operations
- **`+MOD`**: Press and hold specified modifiers
- **`+MOD content`**: Press modifiers, process content with modifiers held
- **Multiple `+MOD`**: Can chain multiple press operations

### Release Operations
- **`-MOD`**: Release specified modifiers
- **Used to end held modifier sequences**

## Examples

```bash
# Text and simple keys
MAP 1 "hello world"
MAP 2 F1
MAP 3 "Line 1\nLine 2\tTab\e"

# Atomic modifier combinations
MAP 4 CTRL-C
MAP 5 CTRL-SHIFT-T
MAP 6 ALT F4
MAP 7 SHIFT F10

# Held modifier sequences
MAP 8 +SHIFT "HELLO WORLD" -SHIFT
MAP 9 +CTRL+ALT "admin mode" -CTRL+ALT
MAP 10 +ALT TAB TAB TAB -ALT

# Gaming/modifier key simulation
MAP 11 down +SHIFT
MAP 11 up -SHIFT
MAP 12 down +CTRL+ALT
MAP 12 up -CTRL+ALT

# Automation sequences
MAP 13 CTRL+L "github.com" ENTER
MAP 14 "Username: " TAB +SHIFT "password123" -SHIFT ENTER
MAP 15 +WIN R -WIN "notepad" ENTER

# Press/release events
MAP 16 down "Key pressed"
MAP 16 up "Key released"
MAP 17 down CTRL-C
MAP 17 up CTRL-V
MAP 18 down +SHIFT
MAP 18 up -SHIFT

# Temporary modifier states
MAP 19 +SHIFT "CAPS TEXT" -SHIFT " normal text"

# Multi-step workflows
MAP 20 +CTRL+SHIFT DEL -CTRL+SHIFT "Task Manager opened"
MAP 21 CTRL+T "new tab" TAB "github.com" ENTER

# Gaming macros with held modifiers
MAP 22 +CTRL Q W E R -CTRL
MAP 23 down +ALT
MAP 23 up -ALT

# Text expansion with formatting
MAP 24 "Email: " "user@domain.com" TAB "Subject: " +SHIFT "URGENT" -SHIFT
```

## Modifier Behavior Summary

| Pattern | Example | Modifier Behavior | Content Behavior |
|---------|---------|-------------------|------------------|
| `MOD-key` | `CTRL-C` | Press+Release | Single key atomic |
| `MOD key` | `CTRL TAB` | Press+Release | Next token atomic |
| `+MOD` | `+CTRL` | Press+Hold | N/A |
| `+MOD content` | `+CTRL "text"` | Press+Hold | All content affected |
| `-MOD` | `-CTRL` | Release | N/A |

## Supported Modifiers

- `CTRL` - Control key
- `ALT` - Alt key  
- `SHIFT` - Shift key
- `WIN` or `GUI` - Windows/GUI/Meta key

## Supported Special Keys

- **Function Keys:** `F1` through `F12`
- **Navigation:** `UP` `DOWN` `LEFT` `RIGHT` `HOME` `END` `PAGEUP` `PAGEDOWN`
- **Editing:** `ENTER` `TAB` `ESC` `BACKSPACE` `DELETE`
- **Other:** `SPACE`

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

