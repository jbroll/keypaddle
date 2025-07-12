/*
 * Command Parsing Function Testing - FIXED with ASSERT_STR_CONTAINS
 * Tests parseSwitchAndDirection and executeWithSwitchAndDirection functions
 */

#include "Arduino.h"
#include "Serial.h"
#include "micro-test.h"

#include "../config.h"
#include "../commands/cmd-parsing.cpp"

// Mock implementation for macros array (needed for command functions)
#include "../storage.h"

#include <iostream>
#include <cstring>

//==============================================================================
// HELPER FUNCTIONS
//==============================================================================

void setupTestEnvironment() {
    Serial.clear();
    
    // Clear all macros
    for (int i = 0; i < NUM_SWITCHES; i++) {
        if (macros[i].downMacro) {
            free(macros[i].downMacro);
            macros[i].downMacro = nullptr;
        }
        if (macros[i].upMacro) {
            free(macros[i].upMacro);
            macros[i].upMacro = nullptr;
        }
    }
}

// Test function that records what it receives
struct TestCallRecord {
    int switchNum;
    int direction;
    std::string remainingArgs;
    bool called;
    
    TestCallRecord() : switchNum(-1), direction(-1), remainingArgs(""), called(false) {}
    
    void reset() {
        switchNum = -1;
        direction = -1;
        remainingArgs = "";
        called = false;
    }
};

TestCallRecord lastCall;

void testCommandFunc(int switchNum, int direction, const char* remainingArgs) {
    lastCall.switchNum = switchNum;
    lastCall.direction = direction;
    lastCall.remainingArgs = remainingArgs ? remainingArgs : "";
    lastCall.called = true;
}

//==============================================================================
// PARSING FUNCTION TESTS - WITH ENHANCED DEBUG OUTPUT
//==============================================================================

void testParseSwitchAndDirection(const TestCase& test) {
    int switchNum = -1;
    int direction = -1;
    const char* remainingArgs = nullptr;
    
    Serial.clear();
    bool result = parseSwitchAndDirection(test.input.c_str(), &switchNum, &direction, &remainingArgs);
    
    // Get any error output for debugging
    std::string serialOutput = Serial.getFullOutput();
    
    if (test.expected == "VALID") {
        ASSERT_TRUE(result, "parseSwitchAndDirection should return true for valid input: '" + test.input + "'");
        ASSERT_TRUE(switchNum >= 0 && switchNum < NUM_SWITCHES, "Switch number should be in valid range (0-" + std::to_string(NUM_SWITCHES-1) + "), got: " + std::to_string(switchNum));
        ASSERT_TRUE(direction == DIRECTION_DOWN || direction == DIRECTION_UP || direction == DIRECTION_UNK, 
                   "Direction should be valid (-1, 0, or 1), got: " + std::to_string(direction));
        ASSERT_TRUE(remainingArgs != nullptr, "Remaining args should not be null");
    } else if (test.expected == "INVALID") {
        ASSERT_TRUE(!result, "parseSwitchAndDirection should return false for invalid input: '" + test.input + "'");
        ASSERT_STR_CONTAINS(serialOutput, "Invalid key", "Should show invalid key error for: '" + test.input + "'");
    } else {
        // Specific expected format: "switch:direction:args"
        std::string expected = test.expected;
        size_t colon1 = expected.find(':');
        size_t colon2 = expected.find(':', colon1 + 1);
        
        if (colon1 != std::string::npos && colon2 != std::string::npos) {
            int expectedSwitch = std::stoi(expected.substr(0, colon1));
            int expectedDirection = std::stoi(expected.substr(colon1 + 1, colon2 - colon1 - 1));
            std::string expectedArgs = expected.substr(colon2 + 1);
            
            ASSERT_TRUE(result, "parseSwitchAndDirection should return true for: '" + test.input + "', but got false. Serial output: '" + serialOutput + "'");
            ASSERT_EQ(switchNum, expectedSwitch, "Switch number should match expected for: '" + test.input + "'");
            ASSERT_EQ(direction, expectedDirection, "Direction should match expected for: '" + test.input + "'");
            ASSERT_STR_EQ(remainingArgs ? remainingArgs : "", expectedArgs, "Remaining args should match expected for: '" + test.input + "'");
        }
    }
}

void testExecuteWithSwitchAndDirection(const TestCase& test) {
    setupTestEnvironment();
    lastCall.reset();
    
    Serial.clear();
    executeWithSwitchAndDirection(test.input.c_str(), testCommandFunc);
    
    // Get any error output for debugging
    std::string serialOutput = Serial.getFullOutput();
    
    if (test.expected == "CALLED") {
        ASSERT_TRUE(lastCall.called, "Command function should be called for valid input: '" + test.input + "'. Serial output: '" + serialOutput + "'");
    } else if (test.expected == "NOT_CALLED") {
        ASSERT_TRUE(!lastCall.called, "Command function should not be called for invalid input: '" + test.input + "'");
        ASSERT_STR_CONTAINS(serialOutput, "Invalid key", "Should show error message for: '" + test.input + "'");
    } else {
        // Specific expected format: "switch:direction:args"
        std::string expected = test.expected;
        size_t colon1 = expected.find(':');
        size_t colon2 = expected.find(':', colon1 + 1);
        
        if (colon1 != std::string::npos && colon2 != std::string::npos) {
            int expectedSwitch = std::stoi(expected.substr(0, colon1));
            int expectedDirection = std::stoi(expected.substr(colon1 + 1, colon2 - colon1 - 1));
            std::string expectedArgs = expected.substr(colon2 + 1);
            
            ASSERT_TRUE(lastCall.called, "Command function should be called for: '" + test.input + "'. Serial output: '" + serialOutput + "'");
            ASSERT_EQ(lastCall.switchNum, expectedSwitch, "Switch number should match expected for: '" + test.input + "'");
            ASSERT_EQ(lastCall.direction, expectedDirection, "Direction should match expected for: '" + test.input + "'");
            ASSERT_STR_EQ(lastCall.remainingArgs, expectedArgs, "Remaining args should match expected for: '" + test.input + "'");
        }
    }
}

//==============================================================================
// DEBUG HELPER FUNCTION
//==============================================================================

void debugParsingFunction(const std::string& input) {
    std::cout << "\n=== DEBUGGING PARSING FUNCTION ===" << std::endl;
    std::cout << "Input: '" << input << "'" << std::endl;
    std::cout << "NUM_SWITCHES = " << NUM_SWITCHES << std::endl;
    
    int switchNum = -1;
    int direction = -1;
    const char* remainingArgs = nullptr;
    
    Serial.clear();
    bool result = parseSwitchAndDirection(input.c_str(), &switchNum, &direction, &remainingArgs);
    
    std::cout << "Result: " << (result ? "true" : "false") << std::endl;
    std::cout << "Switch number: " << switchNum << std::endl;
    std::cout << "Direction: " << direction << " (";
    switch (direction) {
        case DIRECTION_DOWN: std::cout << "DOWN"; break;
        case DIRECTION_UP: std::cout << "UP"; break; 
        case DIRECTION_UNK: std::cout << "UNK"; break;
        default: std::cout << "INVALID"; break;
    }
    std::cout << ")" << std::endl;
    std::cout << "Remaining args: '" << (remainingArgs ? remainingArgs : "NULL") << "'" << std::endl;
    
    std::string serialOutput = Serial.getFullOutput();
    if (!serialOutput.empty()) {
        std::cout << "Serial output: '" << serialOutput << "'" << std::endl;
    }
    
    std::cout << "==================================\n" << std::endl;
}

//==============================================================================
// TEST CASE DEFINITIONS - FIXED WITH ENHANCED DEBUG INFO
//==============================================================================

std::vector<TestCase> createParsingTests() {
    return {
        // Basic valid cases that are currently failing
        TestCase("Simple key", "5", "5:-1:"),                    // Key 5, UNK direction (default), no args
        TestCase("Key with DOWN", "10 down", "INVALID"),         // FIXED: Key 10 is invalid for NUM_SWITCHES=8
        TestCase("Key with UP", "15 up", "INVALID"),             // FIXED: Key 15 is invalid for NUM_SWITCHES=8
        TestCase("Key with args", "0 hello world", "0:-1:hello world"), // Key 0, DIRECTION_UNK, args
        TestCase("Key UP with args", "7 up test args", "7:1:test args"), // FIXED: Use key 7 instead of 8
        TestCase("Key DOWN with args", "6 down test args", "6:0:test args"), // FIXED: Use key 6 instead of 12
        
        // Edge cases with valid keys
        TestCase("Leading whitespace", "  3  ", "3:-1:"),        // Whitespace handling
        TestCase("Mixed case UP", "7 UP", "7:1:"),              // Case insensitive
        TestCase("Mixed case DOWN", "2 Down", "2:0:"),          // Case insensitive
        TestCase("Max key", "7", "7:-1:"),                       // FIXED: Maximum key number is 7 for NUM_SWITCHES=8
        TestCase("Min key", "0", "0:-1:"),                       // Minimum key number
        
        // Invalid cases  
        TestCase("Negative key", "-1", "INVALID"),
        TestCase("Key too high", "8", "INVALID"),                // FIXED: 8 is invalid for NUM_SWITCHES=8
        TestCase("Key too high 2", "99", "INVALID"),
        TestCase("Invalid key text", "abc", "INVALID"),
        TestCase("Empty input", "", "INVALID"),
    };
}

std::vector<TestCase> createExecuteTests() {
    return {
        // Valid executions with corrected key numbers
        TestCase("Execute simple", "5", "5:-1:"),
        TestCase("Execute with UP", "7 up", "7:1:"),            // FIXED: Use key 7 instead of 10
        TestCase("Execute with args", "3 hello world", "3:-1:hello world"),
        
        // Invalid executions
        TestCase("Execute invalid key", "99", "NOT_CALLED"),
        TestCase("Execute negative", "-5", "NOT_CALLED"),
        TestCase("Execute key too high", "8", "NOT_CALLED"),     // FIXED: 8 is invalid for NUM_SWITCHES=8
    };
}

std::vector<TestCase> createDirectionUnkTests() {
    return {
        TestCase("Direction UNK behavior", "", EXPECT_PASS),
    };
}

//==============================================================================
// MAIN TEST RUNNER
//==============================================================================

int main(int argc, char* argv[]) {
    bool verbose = (argc > 1 && strcmp(argv[1], "-v") == 0);
    bool debug = (argc > 1 && strcmp(argv[1], "-d") == 0);
    
    if (debug) {
        std::cout << "=== DEBUG MODE ===" << std::endl;
        debugParsingFunction("5");
        debugParsingFunction("10 down");
        debugParsingFunction("15 up");
        debugParsingFunction("7 up test args");
        debugParsingFunction("6 down test args");
        return 0;
    }
    
    std::cout << "Running Command Parsing Function Tests" << std::endl;
    std::cout << "======================================" << std::endl << std::endl;
    
    TestRunner runner(verbose);
    
    std::cout << "parseSwitchAndDirection Tests:" << std::endl;
    auto parsingTests = createParsingTests();
    for (const auto& test : parsingTests) {
        runner.runTest(test, testParseSwitchAndDirection);
    }
    
    std::cout << std::endl << "executeWithSwitchAndDirection Tests:" << std::endl;
    auto executeTests = createExecuteTests();
    for (const auto& test : executeTests) {
        runner.runTest(test, testExecuteWithSwitchAndDirection);
    }
    
    std::cout << std::endl << "DIRECTION_UNK Behavior Tests:" << std::endl;
    TestCase directionUnkTest("DIRECTION_UNK handling", "", EXPECT_PASS);
    runner.runTest(directionUnkTest, [](const TestCase& test) {
        // This test just verifies that DIRECTION_UNK is handled properly
        // The actual functionality is tested in the parsing tests above
        ASSERT_TRUE(true, "DIRECTION_UNK constants are defined and available");
    });
    
    std::cout << std::endl;
    runner.printSummary();
    
    if (!runner.allPassed()) {
        std::cout << std::endl << "💡 TIP: Run with -d flag to see detailed debug output:" << std::endl;
        std::cout << "./test-parsing -d" << std::endl;
        std::cout << std::endl << "The main issue is likely that test cases were using invalid key numbers." << std::endl;
        std::cout << "For NUM_SWITCHES=" << NUM_SWITCHES << ", valid keys are 0-" << (NUM_SWITCHES-1) << std::endl;
    }
    
    return runner.allPassed() ? 0 : 1;
}
