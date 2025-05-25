# Macro Encode/Decode Testing

Simple round-trip testing for the macro encode/decode functionality.

## Usage

```bash
cd tests
make test      # Run tests
make test-roundtrip && ./test-roundtrip -v   # Verbose output
make clean
```

## What it does

1. **Encode** macro command → UTF-8+ bytes
2. **Decode** bytes → human-readable format  
3. **Compare** with expected result

Tests cover basic text, special keys, modifiers, escape sequences, and error cases.