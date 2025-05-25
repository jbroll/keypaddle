/*
 * Encode/Decode Testing for Macro System
 * Tests that encode->decode produces the expected output
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
// ENCODE->DECODE TEST FUNCTION
//==============================================================================

void performEncodeDecodeTest(const TestCase& test) {
    // Step 1: Encode
    MacroEncodeResult encodeResult = macroEncode(test.input.c_str());
    
    if (encodeResult.error != nullptr) {
        throw std::runtime_error("Encoding failed: " + std::string(encodeResult.error));
    }
    
    if (!encodeResult.utf8Sequence) {
        throw std::runtime_error("Encoding produced null sequence");
    }
    
    // Step 2: Decode
    uint8_t* bytes = (uint8_t*)encodeResult.utf8Sequence;
    uint16_t length = strlen(encodeResult.utf8Sequence);
    
    String decoded = macroDecode(bytes, length);
    std::string actual = decoded;
    
    // Clean up
    free(encodeResult.utf8Sequence);
    
    // Step 3: Assert the result matches expected
    ASSERT_STR_EQ(actual, test.expected, "Decoded output differs from expected");
}

//==============================================================================
// TEST CASES
//==============================================================================

std::vector<TestCase> createBasicTests() {
    return {
        // Simple atomic operations - these expand to explicit press/release
        TestCase("Simple CTRL C", "CTRL C", "+CTRL \"c\" -CTRL"),
        TestCase("SHIFT F1", "SHIFT F1", "+SHIFT F1 -SHIFT"),
        TestCase("ALT TAB", "ALT TAB", "+ALT \"\\t\" -ALT"),
        TestCase("Multi-modifier", "CTRL+SHIFT T", "+CTRL+SHIFT \"t\" -CTRL+SHIFT"),
        
        // Keywords that become characters in quoted strings
        TestCase("ENTER keyword", "ENTER", "\"\\n\""),
        TestCase("TAB keyword", "TAB", "\"\\t\""),
        TestCase("SPACE keyword", "SPACE", "\" \""),
        TestCase("ESC keyword", "ESC", "\"\\e\""),
        TestCase("BACKSPACE keyword", "BACKSPACE", "\"\\b\""),
        
        // Function keys remain as keywords
        TestCase("Function key", "F1", "F1"),
        TestCase("Function key F12", "F12", "F12"),
        
        // Navigation keys remain as keywords
        TestCase("Arrow key", "UP", "UP"),
        TestCase("Navigation key", "HOME", "HOME"),
        TestCase("Delete key", "DELETE", "DELETE"),
        
        // Simple text - always quoted even for single characters
        TestCase("Simple text", "\"hello\"", "\"hello\""),
        TestCase("Single character", "\"a\"", "\"a\""),
        TestCase("Single space", "\" \"", "\" \""),
        
        // Explicit press/release should pass through
        TestCase("Press CTRL", "+CTRL", "+CTRL"),
        TestCase("Release CTRL", "-CTRL", "-CTRL"),
        TestCase("Press multiple", "+CTRL+SHIFT", "+CTRL+SHIFT"),
        TestCase("Release multiple", "-CTRL+SHIFT", "-CTRL+SHIFT"),
    };
}

std::vector<TestCase> createAdvancedTests() {
    return {
        // Complex sequences with mixed content
        TestCase("Hold and type", "+SHIFT \"HELLO\" -SHIFT \" world\"", "+SHIFT \"HELLO\" -SHIFT \" world\""),
        TestCase("Copy and paste", "CTRL C \"copied\" CTRL V", "+CTRL \"c\" -CTRL \"copied\" +CTRL \"v\" -CTRL"),
        
        // Escape sequences remain as escapes in quoted strings
        TestCase("Newline escape", "\"line1\\nline2\"", "\"line1\\nline2\""),
        TestCase("Tab escape", "\"text\\ttabbed\"", "\"text\\ttabbed\""),
        TestCase("All escapes", "\"\\n\\r\\t\\a\\e\\\"\\\\\"", "\"\\n\\r\\t\\a\\e\\\"\\\\\""),
        
        // Mixed keywords and text
        TestCase("Keyword with text", "CTRL \"abc\"", "+CTRL \"abc\" -CTRL"),
        TestCase("Text with navigation", "\"text\" UP \"more\"", "\"text\" UP \"more\""),
        TestCase("Complex sequence", "CTRL A \"select\\nall\" ENTER", "+CTRL \"a\" -CTRL \"select\\nall\" \"\\n\""),
        
        // Multiple spaces and formatting
        TestCase("Multiple spaces", "\"hello   world\"", "\"hello   world\""),
        TestCase("Mixed whitespace", "\"tab\\there\\nnewline\"", "\"tab\\there\\nnewline\""),
        
        // Modifier combinations
        TestCase("Triple modifier", "CTRL+ALT+SHIFT DELETE", "+CTRL+ALT+SHIFT DELETE -CTRL+ALT+SHIFT"),
        TestCase("Modifier with function key", "CTRL F1", "+CTRL F1 -CTRL"),
        
        // Edge cases
        TestCase("Just quotes", "\"\"", "\"\""),
        TestCase("Special chars", "\"!@#$%^&*()\"", "\"!@#$%^&*()\""),
    };
}

std::vector<TestCase> createErrorTests() {
    return {
        // These should fail during encoding
        TestCase("Unknown keyword", "UNKNOWN_KEY", EXPECT_FAIL),
        TestCase("Empty input", "", EXPECT_FAIL),
        TestCase("Modifier without key", "CTRL+SHIFT", EXPECT_FAIL),
        TestCase("Empty modifier", "+", EXPECT_FAIL),
    };
}

//==============================================================================
// MAIN TEST RUNNER
//==============================================================================

int main(int argc, char* argv[]) {
    bool verbose = (argc > 1 && strcmp(argv[1], "-v") == 0);
    
    std::cout << "Running Encode/Decode Tests for Macro System" << std::endl;
    std::cout << "=============================================" << std::endl << std::endl;
    
    TestRunner runner(verbose);
    
    std::cout << "Basic Tests:" << std::endl;
    auto basicTests = createBasicTests();
    for (const auto& test : basicTests) {
        runner.runTest(test, performEncodeDecodeTest);
    }
    
    std::cout << std::endl << "Advanced Tests:" << std::endl;
    auto advancedTests = createAdvancedTests();
    for (const auto& test : advancedTests) {
        runner.runTest(test, performEncodeDecodeTest);
    }
    
    std::cout << std::endl << "Error Tests:" << std::endl;
    auto errorTests = createErrorTests();
    for (const auto& test : errorTests) {
        runner.runTest(test, performEncodeDecodeTest);
    }
    
    std::cout << std::endl;
    runner.printSummary();
    return runner.allPassed() ? 0 : 1;
}