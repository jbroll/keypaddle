/*
 * Round-trip Testing for Macro Encode/Decode
 * Tests that encode->decode produces the same logical result
 */

#include "Arduino.h"
#include "micro-test.h"

// Include the actual implementation files from parent directory
#include "../map-parser-tables.h"
#include "../macro-encode.h"
#include "../macro-decode.h"

#include <iostream>
#include <cstring>

//==============================================================================
// ROUND-TRIP TEST FUNCTION
//==============================================================================

std::pair<std::string, std::string> performRoundTrip(const std::string& input) {
    // Step 1: Encode
    MacroEncodeResult encodeResult = macroEncode(input.c_str());
    
    if (encodeResult.error != nullptr) {
        return std::make_pair("", std::string(encodeResult.error));
    }
    
    if (!encodeResult.utf8Sequence) {
        return std::make_pair("", "Encoding produced null sequence");
    }
    
    // Step 2: Decode
    uint8_t* bytes = (uint8_t*)encodeResult.utf8Sequence;
    uint16_t length = strlen(encodeResult.utf8Sequence);
    
    String decoded = macroDecode(bytes, length);
    
    // Clean up and return result
    std::string result = decoded;
    free(encodeResult.utf8Sequence);
    
    return std::make_pair(result, "");
}

//==============================================================================
// TEST CASES
//==============================================================================

std::vector<TestCase> createBasicTests() {
    return {
        // Simple text
        TestCase("Simple text", "hello"),
        TestCase("Quoted text", "\"hello world\""),
        TestCase("Single char", "a"),
        
        // Special keys
        TestCase("Function key", "F1"),
        TestCase("Arrow key", "UP"),
        TestCase("Enter key", "ENTER"),
        TestCase("Tab key", "TAB"),
        
        // Atomic modifiers
        TestCase("Simple CTRL+C", "CTRL+C"),
        TestCase("SHIFT+F1", "SHIFT+F1"),
        TestCase("ALT+TAB", "ALT+TAB"),
        
        // Explicit press/release
        TestCase("Press CTRL", "+CTRL"),
        TestCase("Release CTRL", "-CTRL"),
        TestCase("Press multiple", "+CTRL+SHIFT"),
        TestCase("Release multiple", "-CTRL+SHIFT"),
        
        // Mixed sequences
        TestCase("Text and key", "\"hello\" ENTER"),
        TestCase("Key and text", "F1 \"help\""),
        
        // Escape sequences
        TestCase("Newline escape", "\"line1\\nline2\""),
        TestCase("Tab escape", "\"text\\ttabbed\""),
        TestCase("Quote escape", "\"say \\\"hello\\\"\""),
        
        // Error cases
        TestCase("Unknown keyword", "UNKNOWN_KEY", false),
        TestCase("Empty input", "", false),
    };
}

std::vector<TestCase> createAdvancedTests() {
    return {
        // Complex sequences
        TestCase("Hold and release", "+SHIFT \"HELLO\" -SHIFT \" world\""),
        TestCase("Multi-modifier chain", "CTRL+SHIFT+ALT"),
        TestCase("Mixed operations", "CTRL+C \"copied\" CTRL+V"),
        
        // Normalization cases (where output differs from input)
        TestCase("Space normalization", " CTRL+C ", "CTRL+C"),
        TestCase("Multiple spaces", "F1  \"help\"", "F1 \"help\""),
        
        // Edge cases
        TestCase("Just modifiers", "CTRL+SHIFT"),
        TestCase("Modifier only", "+SHIFT"),
        TestCase("Release only", "-SHIFT"),
        TestCase("Empty quotes", "\"\""),
        
        // Complex escapes
        TestCase("All escapes", "\"\\n\\r\\t\\\"\\\\\""),
        TestCase("Mixed content", "CTRL+A \"select\\nall\" ENTER"),
    };
}

//==============================================================================
// MAIN TEST RUNNER
//==============================================================================

int main(int argc, char* argv[]) {
    bool verbose = (argc > 1 && strcmp(argv[1], "-v") == 0);
    
    TestRunner runner(verbose);
    
    // Basic tests
    auto basicTests = createBasicTests();
    for (const auto& test : basicTests) {
        runner.runRoundTripTest(test, performRoundTrip);
    }
    
    // Advanced tests
    auto advancedTests = createAdvancedTests();
    for (const auto& test : advancedTests) {
        runner.runRoundTripTest(test, performRoundTrip);
    }
    
    runner.printSummary();
    return runner.allPassed() ? 0 : 1;
}