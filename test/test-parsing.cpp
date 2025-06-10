/*
 * Command Parsing Function Testing
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
// PARSING FUNCTION TESTS
//==============================================================================

void testParseSwitchAndDirection(const TestCase& test) {
    int switchNum = -1;
    int direction = -1;
    const char* remainingArgs = nullptr;
    
    Serial.clear();
    bool result = parseSwitchAndDirection(test.input.c_str(), &switchNum, &direction, &remainingArgs);
    
    if (test.expected == "VALID") {
        ASSERT_TRUE(result, "parseSwitchAndDirection should return true for valid input");
        ASSERT_TRUE(switchNum >= 0 && switchNum < NUM_SWITCHES, "Switch number should be in valid range");
        ASSERT_TRUE(direction == DIRECTION_DOWN || direction == DIRECTION_UP || direction == DIRECTION_UNK, 
                   "Direction should be valid");
        ASSERT_TRUE(remainingArgs != nullptr, "Remaining args should not be null");
    } else if (test.expected == "INVALID") {
        ASSERT_TRUE(!result, "parseSwitchAndDirection should return false for invalid input");
        ASSERT_TRUE(Serial.containsOutput("Invalid key"), "Should show invalid key error");
    } else {
        // Specific expected format: "switch:direction:args"
        std::string expected = test.expected;
        size_t colon1 = expected.find(':');
        size_t colon2 = expected.find(':', colon1 + 1);
        
        if (colon1 != std::string::npos && colon2 != std::string::npos) {
            int expectedSwitch = std::stoi(expected.substr(0, colon1));
            int expectedDirection = std::stoi(expected.substr(colon1 + 1, colon2 - colon1 - 1));
            std::string expectedArgs = expected.substr(colon2 + 1);
            
            ASSERT_TRUE(result, "parseSwitchAndDirection should return true");
            ASSERT_EQ(switchNum, expectedSwitch, "Switch number should match expected");
            ASSERT_EQ(direction, expectedDirection, "Direction should match expected");
            ASSERT_STR_EQ(remainingArgs ? remainingArgs : "", expectedArgs, "Remaining args should match expected");
        }
    }
}

void testExecuteWithSwitchAndDirection(const TestCase& test) {
    setupTestEnvironment();
    lastCall.reset();
    
    Serial.clear();
    executeWithSwitchAndDirection(test.input.c_str(), testCommandFunc);
    
    if (test.expected == "CALLED") {
        ASSERT_TRUE(lastCall.called, "Command function should be called for valid input");
    } else if (test.expected == "NOT_CALLED") {
        ASSERT_TRUE(!lastCall.called, "Command function should not be called for invalid input");
        ASSERT_TRUE(Serial.containsOutput("Invalid key"), "Should show error message");
    } else {
        // Specific expected format: "switch:direction:args"
        std::string expected = test.expected;
        size_t colon1 = expected.find(':');
        size_t colon2 = expected.find(':', colon1 + 1);
        
        if (colon1 != std::string::npos && colon2 != std::string::npos) {
            int expectedSwitch = std::stoi(expected.substr(0, colon1));
            int expectedDirection = std::stoi(expected.substr(colon1 + 1, colon2 - colon1 - 1));
            std::string expectedArgs = expected.substr(colon2 + 1);
            
            ASSERT_TRUE(lastCall.called, "Command function should be called");
            ASSERT_EQ(lastCall.switchNum, expectedSwitch, "Switch number should match expected");
            ASSERT_EQ(lastCall.direction, expectedDirection, "Direction should match expected");
            ASSERT_STR_EQ(lastCall.remainingArgs, expectedArgs, "Remaining args should match expected");
        }
    }
}

//==============================================================================
// TEST CASE DEFINITIONS
//==============================================================================

std::vector<TestCase> createParsingTests() {
    return {
        // Basic valid cases
        TestCase("Simple key", "5", "5:-1:"),                    // Key 5, UNK direction (default), no args
        TestCase("Key with DOWN", "10 down", "10:0:"),          // Key 10, DOWN direction (explicit), no args
        TestCase("Key with UP", "15 up", "15:1:"),              // Key 15, UP direction, no args
        TestCase("Key with args", "0 hello world", "0:-1:hello world"), // Key 0, DIRECTION_UNK, args
        TestCase("Key UP with args", "8 up test args", "8:1:test args"), // Key 8, UP, args
        TestCase("Key DOWN with args", "12 down test args", "12:0:test args"), // Key 12, DOWN, args
        
        // Edge cases
        TestCase("Leading whitespace", "  3  ", "3:-1:"),        // Whitespace handling
        TestCase("Mixed case UP", "7 UP", "7:1:"),              // Case insensitive
        TestCase("Mixed case DOWN", "2 Down", "2:0:"),          // Case insensitive
        TestCase("Max key", "23", "23:-1:"),                     // Maximum key number
        TestCase("Min key", "0", "0:-1:"),                       // Minimum key number
        
        // Invalid cases  
        TestCase("Negative key", "-1", "INVALID"),
        TestCase("Key too high", "24", "INVALID"),
        TestCase("Key too high 2", "99", "INVALID"),
        TestCase("Invalid key text", "abc", "INVALID"),
        TestCase("Empty input", "", "INVALID"),
    };
}

std::vector<TestCase> createExecuteTests() {
    return {
        // Valid executions
        TestCase("Execute simple", "5", "5:-1:"),
        TestCase("Execute with UP", "10 up", "10:1:"),
        TestCase("Execute with args", "3 hello world", "3:-1:hello world"),
        
        // Invalid executions
        TestCase("Execute invalid key", "99", "NOT_CALLED"),
        TestCase("Execute negative", "-5", "NOT_CALLED"),
    };
}

std::vector<TestCase> createDirectionUnkTests() {
    return {
        TestCase("SHOW with UNK", "", EXPECT_PASS),
        TestCase("CLEAR with UNK", "", EXPECT_PASS),
        TestCase("MAP with UNK", "", EXPECT_PASS),
    };
}

//==============================================================================
// MAIN TEST RUNNER
//==============================================================================

int main(int argc, char* argv[]) {
    bool verbose = (argc > 1 && strcmp(argv[1], "-v") == 0);
    
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
    TestCase showUnkTest("SHOW with DIRECTION_UNK", "", EXPECT_PASS);
    TestCase clearUnkTest("CLEAR with DIRECTION_UNK", "", EXPECT_PASS);
    TestCase mapUnkTest("MAP with DIRECTION_UNK", "", EXPECT_PASS);
    
    std::cout << std::endl;
    runner.printSummary();
    return runner.allPassed() ? 0 : 1;
}
