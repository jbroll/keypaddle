# Macro Decoder Behavior Specification

## Overview

The macro decoder converts UTF-8+ encoded byte sequences back into human-readable MAP command syntax. This document specifies the expected behavior and output format.

## Core Principles

### 1. **Characters Over Keywords**
The decoder prioritizes character representation over keyword representation whenever possible:
- `ENTER` keyword → `"\n"` (newline character)
- `TAB` keyword → `"\t"` (tab character)  
- `SPACE` keyword → `" "` (space character)
- `ESC` keyword → `"\e"` (escape character)
- `BACKSPACE` keyword → `"\b"` (backspace character)

### 2. **Always Quote Character Data**
All character sequences are wrapped in double quotes, even single characters:
- Single character: `"a"`
- Multiple characters: `"hello world"`
- Special characters: `"line1\nline2"`

### 3. **Preserve Keywords for Non-Character Keys**
Only keys without meaningful character representations remain as keywords:
- **Function keys**: `F1`, `F2`, ..., `F12`
- **Navigation keys**: `UP`, `DOWN`, `LEFT`, `RIGHT`, `HOME`, `END`, `PAGEUP`, `PAGEDOWN`, `DELETE`

### 4. **Atomic Operations Are Not Preserved**
Atomic operations from the input are expanded to their press/key/release sequences:
- Input: `CTRL C` (atomic)
- Encoded: press CTRL, send 'c', release CTRL
- Decoded: `+CTRL "c" -CTRL` (explicit sequence)

## Decoding Rules

### Control Code Handling

UTF-8+ control codes are decoded to their explicit forms:

| Control Code | Decoded Output |
|--------------|----------------|
| `UTF8_PRESS_CTRL` | `+CTRL` |
| `UTF8_PRESS_ALT` | `+ALT` |
| `UTF8_PRESS_SHIFT` | `+SHIFT` |
| `UTF8_PRESS_CMD` | `+WIN` |
| `UTF8_RELEASE_CTRL` | `-CTRL` |
| `UTF8_RELEASE_ALT` | `-ALT` |
| `UTF8_RELEASE_SHIFT` | `-SHIFT` |
| `UTF8_RELEASE_CMD` | `-WIN` |
| `UTF8_PRESS_MULTI` + mask | `+CTRL+SHIFT` (example) |
| `UTF8_RELEASE_MULTI` + mask | `-CTRL+SHIFT` (example) |

### Character Grouping

The decoder groups consecutive non-control characters into single quoted strings:

**Example 1: Simple grouping**
```
Input bytes: ['h', 'e', 'l', 'l', 'o']
Output: "hello"
```

**Example 2: Grouping stops at control codes**
```
Input bytes: ['h', 'i', UTF8_PRESS_CTRL, 'c', UTF8_RELEASE_CTRL]
Output: "hi" +CTRL "c" -CTRL
```

**Example 3: Grouping stops at function keys**
```
Input bytes: ['t', 'e', 's', 't', KEY_F1, 'h', 'e', 'l', 'p']
Output: "test" F1 "help"
```

### Escape Sequence Handling

Special characters within quoted strings use escape sequences:

| Character | Escape Sequence |
|-----------|-----------------|
| `"` | `\"` |
| `\` | `\\` |
| Newline (`\n`) | `\n` |
| Carriage Return (`\r`) | `\r` |
| Tab (`\t`) | `\t` |
| Bell (`\a`) | `\a` |
| Escape (0x1B) | `\e` |
| Backspace (0x08) | `\b` |

## Complete Examples

### Example 1: Simple Atomic Operation
```
Input command: CTRL C
Encoded bytes: [UTF8_PRESS_CTRL, 'c', UTF8_RELEASE_CTRL]
Decoded output: +CTRL "c" -CTRL
```

### Example 2: Multi-Modifier Atomic Operation
```
Input command: CTRL+SHIFT T
Encoded bytes: [UTF8_PRESS_MULTI, 0x03, 't', UTF8_RELEASE_MULTI, 0x03]
Decoded output: +CTRL+SHIFT "t" -CTRL+SHIFT
```
*Note: 0x03 = MULTI_CTRL | MULTI_SHIFT*

### Example 3: Mixed Keywords and Text
```
Input command: CTRL A "select all" ENTER
Encoded bytes: [UTF8_PRESS_CTRL, 'a', UTF8_RELEASE_CTRL, 's','e','l','e','c','t',' ','a','l','l', '\n']
Decoded output: +CTRL "a" -CTRL "select all" "\n"
```

### Example 4: Function Keys Remain Keywords
```
Input command: ALT F4
Encoded bytes: [UTF8_PRESS_ALT, KEY_F4, UTF8_RELEASE_ALT]
Decoded output: +ALT F4 -ALT
```

### Example 5: Special Characters Become Quoted
```
Input command: TAB SPACE ESC
Encoded bytes: ['\t', ' ', 0x1B]
Decoded output: "\t \e"
```

### Example 6: Explicit Press/Release Operations
```
Input command: +SHIFT "HELLO" -SHIFT
Encoded bytes: [UTF8_PRESS_SHIFT, 'H','E','L','L','O', UTF8_RELEASE_SHIFT]
Decoded output: +SHIFT "HELLO" -SHIFT
```

### Example 7: Navigation Keys Remain Keywords
```
Input command: "text" UP DOWN "more"
Encoded bytes: ['t','e','x','t', KEY_UP_ARROW, KEY_DOWN_ARROW, 'm','o','r','e']
Decoded output: "text" UP DOWN "more"
```

## Edge Cases

### Empty Input
```
Input bytes: []
Decoded output: ""
```

### Only Control Codes
```
Input bytes: [UTF8_PRESS_CTRL, UTF8_RELEASE_CTRL]
Decoded output: +CTRL -CTRL
```

### Mixed Control and Data
```
Input bytes: [UTF8_PRESS_CTRL, 'a', 'b', 'c', UTF8_RELEASE_CTRL]
Decoded output: +CTRL "abc" -CTRL
```

### Escape Sequences in Strings
```
Input command: "line1\nline2\ttabbed"
Encoded bytes: ['l','i','n','e','1','\n','l','i','n','e','2','\t','t','a','b','b','e','d']
Decoded output: "line1\nline2\ttabbed"
```

## Token Separation

Tokens in the decoded output are separated by single spaces. The decoder automatically handles spacing between:
- Control codes and character strings
- Keywords and character strings  
- Multiple control codes
- Multiple keywords

## Consistency Guarantees

1. **Idempotent for explicit operations**: `+CTRL "a" -CTRL` → encode → decode → `+CTRL "a" -CTRL`
2. **Character preference**: Any byte that can be represented as a character will be, unless it's a function key or navigation key
3. **Grouping optimization**: Consecutive characters are always grouped into single quoted strings
4. **Escape preservation**: Escape sequences in the original input are preserved in the decoded output

## Implementation Notes

- The decoder processes bytes sequentially in a single pass
- Control codes immediately terminate character grouping
- Function key detection uses HID code ranges (0x3A-0x45 for F1-F12)
- Navigation key detection uses specific HID code values
- All other bytes are treated as characters and quoted