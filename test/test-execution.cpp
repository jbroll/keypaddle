/*
 * Execution Engine Testing for Macro System
 * Tests that macroEncode->executeUTF8Macro produces the expected keyboard actions
 */

#include "Arduino.h"
#include "Keyboard.h"
#include "micro-test.h"

// Include the actual implementation files from parent directory
#include "../map-parser-tables.h"
#include "../macro-encode.h"
#include "../macro-engine.h"

#include <iostream>
#include <cstring>

MockKeyboard Keyboard;

//==============================================================================
// ENCODE->EXECUTE TEST FUNCTION
//==============================================================================

void performExecutionTest(const TestCase& test) {
    // Step 1: Encode the macro command
    MacroEncodeResult encoded = macroEncode(test.input.c_str());
    
    if (encoded.error != nullptr) {
        ASSERT_FAIL("Encoding failed: " + std::string(encoded.error));
    }
    
    if (!encoded.utf8Sequence) {
        ASSERT_FAIL("Encoding produced null sequence");
    }
    
    // Step 2: Clear keyboard mock and execute
    Keyboard.clearActions();
    executeUTF8Macro((uint8_t*)encoded.utf8Sequence, strlen(encoded.utf8Sequence));
    
    // Step 3: Compare actual vs expected keyboard actions
    std::string actual = Keyboard.toString();
    
    // Clean up
    free(encoded.utf8Sequence);
    
    // Step 4: Assert the result matches expected
    ASSERT_STR_EQ(actual, test.expected, "Keyboard actions differ");
}

//==============================================================================
// TEST CASES
//==============================================================================

std::vector<TestCase> createBasicExecutionTests() {
    return {
        // Simple atomic operations
        TestCase("Simple CTRL C", "CTRL C", "press ctrl write c release ctrl"),
        TestCase("ALT F4", "ALT F4", "press alt write f4 release alt"),
        TestCase("SHIFT A", "SHIFT A", "press shift write a release shift"),
        
        // Multi-modifier atomic operations
        TestCase("Multi-modifier", "CTRL+SHIFT T", "press ctrl press shift write t release ctrl release shift"),
        TestCase("Triple modifier", "CTRL+ALT+SHIFT DELETE", "press ctrl press shift press alt write delete release ctrl release shift release alt"),
        
        // Function keys
        TestCase("Function key", "F1", "write f1"),
        TestCase("F12 key", "F12", "write f12"),
        TestCase("Modifier + Function", "CTRL F1", "press ctrl write f1 release ctrl"),
        
        // Navigation keys
        TestCase("Arrow key", "UP", "write up"),
        TestCase("Home key", "HOME", "write home"),
        TestCase("Delete key", "DELETE", "write delete"),
        TestCase("Shift + Arrow", "SHIFT UP", "press shift write up release shift"),
        
        // Keywords that become characters
        TestCase("Enter keyword", "ENTER", "write \\n"),
        TestCase("Tab keyword", "TAB", "write \\t"),
        TestCase("Space keyword", "SPACE", "write  "),  // Space character
        TestCase("Escape keyword", "ESC", "write \\e"),
        TestCase("Backspace keyword", "BACKSPACE", "write \\b"),
        
        // Simple text
        TestCase("Simple text", "\"hello\"", "write h write e write l write l write o"),
        TestCase("Single character", "\"A\"", "write A"),
        TestCase("Mixed case", "\"HeLLo\"", "write H write e write L write L write o"),
    };
}

std::vector<TestCase> createAdvancedExecutionTests() {
    return {
        // Explicit press/release operations
        TestCase("Press CTRL", "+CTRL", "press ctrl"),
        TestCase("Release CTRL", "-CTRL", "release ctrl"),
        TestCase("Press multiple", "+CTRL+SHIFT", "press ctrl press shift"),
        TestCase("Release multiple", "-CTRL+SHIFT", "release ctrl release shift"),
        
        // Held modifier sequences
        TestCase("Hold and type", "+SHIFT \"HI\" -SHIFT", "press shift write H write I release shift"),
        TestCase("Complex hold", "+CTRL \"abc\" -CTRL", "press ctrl write a write b write c release ctrl"),
        TestCase("Multi-hold", "+CTRL+ALT \"test\" -CTRL+ALT", "press ctrl press alt write t write e write s write t release ctrl release alt"),
        
        // Mixed sequences
        TestCase("Copy paste", "CTRL C \"text\" CTRL V", "press ctrl write c release ctrl write t write e write x write t press ctrl write v release ctrl"),
        TestCase("Text with nav", "\"start\" UP \"end\"", "write s write t write a write r write t write up write e write n write d"),
        
        // Escape sequences in text
        TestCase("Newline in text", "\"line1\\nline2\"", "write l write i write n write e write 1 write \\n write l write i write n write e write 2"),
        TestCase("Tab in text", "\"before\\tafter\"", "write b write e write f write o write r write e write \\t write a write f write t write e write r"),
        TestCase("All escapes", "\"\\n\\r\\t\\e\"", "write \\n write \\r write \\t write \\e"),
        
        // Complex real-world scenarios
        TestCase("Window switch", "ALT TAB", "press alt write \\t release alt"),
        TestCase("Select all", "CTRL A", "press ctrl write a release ctrl"),
        TestCase("Undo", "CTRL Z", "press ctrl write z release ctrl"),
        TestCase("New tab", "CTRL T", "press ctrl write t release ctrl"),
    };
}

std::vector<TestCase> createEdgeCaseTests() {
    return {
        // Empty and minimal cases
        TestCase("Empty quotes", "\"\"", ""),
        TestCase("Just space", "\" \"", "write  "),
        TestCase("Single newline", "\"\\n\"", "write \\n"),
        
        // Special characters
        TestCase("Special chars", "\"!@#$\"", "write ! write @ write # write $"),
        TestCase("Numbers", "\"12345\"", "write 1 write 2 write 3 write 4 write 5"),
        
        // Multiple modifiers with different keys
        TestCase("Different modifier combos", "CTRL A ALT B SHIFT C", "press ctrl write a release ctrl press alt write b release alt press shift write c release shift"),
    };
}

//==============================================================================
// MAIN TEST RUNNER
//==============================================================================

int main(int argc, char* argv[]) {
    bool verbose = (argc > 1 && strcmp(argv[1], "-v") == 0);
    
    std::cout << "Running Execution Engine Tests for Macro System" << std::endl;
    std::cout << "===============================================" << std::endl << std::endl;
    
    TestRunner runner(verbose);
    
    std::cout << "Basic Execution Tests:" << std::endl;
    auto basicTests = createBasicExecutionTests();
    for (const auto& test : basicTests) {
        runner.runTest(test, performExecutionTest);
    }
    
    std::cout << std::endl << "Advanced Execution Tests:" << std::endl;
    auto advancedTests = createAdvancedExecutionTests();
    for (const auto& test : advancedTests) {
        runner.runTest(test, performExecutionTest);
    }
    
    std::cout << std::endl << "Edge Case Tests:" << std::endl;
    auto edgeCaseTests = createEdgeCaseTests();
    for (const auto& test : edgeCaseTests) {
        runner.runTest(test, performExecutionTest);
    }
    
    std::cout << std::endl;
    runner.printSummary();
    return runner.allPassed() ? 0 : 1;
}
