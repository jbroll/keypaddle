/*
 * Serial Interface Testing - Rewritten with ASSERT_STR_CONTAINS
 * Tests command parsing, execution, and response formatting
 */

#include "Arduino.h"
#include "EEPROM.h"
#include "micro-test.h"

// Include the actual implementation files from parent directory
#include "../config.h"
#include "../storage.h"
#include "../macro-encode.h"
#include "../macro-decode.h"
#include "../serial-interface.h"

#include <iostream>
#include <cstring>

//==============================================================================
// TEST HELPER FUNCTIONS
//==============================================================================

void setupTestEnvironment() {
    // Clear all state
    Serial.clear();
    EEPROM.clear();
    
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
    
    // Initialize storage
    setupStorage();
}

void setTestMacro(int keyIndex, const char* downMacro, const char* upMacro = nullptr) {
    if (keyIndex < 0 || keyIndex >= NUM_SWITCHES) return;
    
    // Clear existing macros
    if (macros[keyIndex].downMacro) {
        free(macros[keyIndex].downMacro);
        macros[keyIndex].downMacro = nullptr;
    }
    if (macros[keyIndex].upMacro) {
        free(macros[keyIndex].upMacro);
        macros[keyIndex].upMacro = nullptr;
    }
    
    // Set new macros
    if (downMacro && strlen(downMacro) > 0) {
        macros[keyIndex].downMacro = (char*)malloc(strlen(downMacro) + 1);
        strcpy(macros[keyIndex].downMacro, downMacro);
    }
    
    if (upMacro && strlen(upMacro) > 0) {
        macros[keyIndex].upMacro = (char*)malloc(strlen(upMacro) + 1);
        strcpy(macros[keyIndex].upMacro, upMacro);
    }
}

// Mock implementation for switches (since we're not testing hardware)
uint32_t loopSwitches() {
    return 0; // No switches pressed for testing
}

//==============================================================================
// COMMAND PARSING TESTS
//==============================================================================

void testCommandParsing(const TestCase& test) {
    setupTestEnvironment();
    
    // Test that the command is recognized and doesn't produce "Unknown command"
    Serial.clear();
    processCommand(test.input.c_str());
    
    std::string output = Serial.getFullOutput();
    
    if (test.expected == "UNKNOWN") {
        ASSERT_STR_CONTAINS(output, "Unknown command", "Should show unknown command message");
    } else {
        ASSERT_STR_NOT_CONTAINS(output, "Unknown command", "Should not show unknown command message");
    }
}

//==============================================================================
// HELP COMMAND TESTS
//==============================================================================

void testHelpCommand(const TestCase& test) {
    setupTestEnvironment();
    Serial.clear();
    
    processCommand("HELP");
    
    std::string output = Serial.getFullOutput();
    std::string expectedRange = "Keys: 0-" + std::to_string(NUM_SWITCHES - 1);
    
    // Verify help output contains expected sections
    ASSERT_STR_CONTAINS(output, "Commands:", "Help should show Commands section");
    ASSERT_STR_CONTAINS(output, "HELP", "Help should list HELP command");
    ASSERT_STR_CONTAINS(output, "SHOW", "Help should list SHOW command");
    ASSERT_STR_CONTAINS(output, "MAP", "Help should list MAP command");
    ASSERT_STR_CONTAINS(output, "CLEAR", "Help should list CLEAR command");
    ASSERT_STR_CONTAINS(output, "LOAD", "Help should list LOAD command");
    ASSERT_STR_CONTAINS(output, "SAVE", "Help should list SAVE command");
    ASSERT_STR_CONTAINS(output, "STAT", "Help should list STAT command");
    
    // ENHANCED: This will now show exactly what's wrong with the key range
    ASSERT_STR_CONTAINS(output, expectedRange, "Help should show correct key range");
}

//==============================================================================
// SHOW COMMAND TESTS
//==============================================================================

void testShowCommand(const TestCase& test) {
    setupTestEnvironment();
    
    // Set up some test macros - use valid indices
    setTestMacro(0, "hello");
    setTestMacro(1, "world", "up-world");
    setTestMacro(5, nullptr, "just-up");
    
    Serial.clear();
    processCommand(test.input.c_str());
    
    std::string output = Serial.getFullOutput();
    
    if (test.input.find("SHOW 0") != std::string::npos) {
        ASSERT_STR_CONTAINS(output, "Key 0", "Should show Key 0");
        ASSERT_STR_CONTAINS(output, "DOWN", "Should show DOWN direction");
        ASSERT_STR_CONTAINS(output, "hello", "Should show macro content");
    }
    else if (test.input.find("SHOW 1 UP") != std::string::npos) {
        ASSERT_STR_CONTAINS(output, "Key 1", "Should show Key 1");
        ASSERT_STR_CONTAINS(output, "UP", "Should show UP direction");
        ASSERT_STR_CONTAINS(output, "up-world", "Should show up macro content");
    }
    else if (test.input.find("SHOW 5 UP") != std::string::npos) {
        ASSERT_STR_CONTAINS(output, "Key 5", "Should show Key 5");
        ASSERT_STR_CONTAINS(output, "UP", "Should show UP direction");
        ASSERT_STR_CONTAINS(output, "just-up", "Should show up-only macro");
    }
    else if (test.input.find("SHOW 2") != std::string::npos) {
        ASSERT_STR_CONTAINS(output, "Key 2", "Should show Key 2");
        ASSERT_STR_CONTAINS(output, "(empty)", "Should show empty for unset macro");
    }
    else if (test.input.find("SHOW ALL") != std::string::npos) {
        ASSERT_STR_CONTAINS(output, "0 DOWN:", "Should show all keys");
        ASSERT_STR_CONTAINS(output, "0 UP:", "Should show all directions");
        
        // Check for last valid key instead of hardcoded 23
        std::string lastKeyDown = std::to_string(NUM_SWITCHES - 1) + " DOWN:";
        std::string lastKeyUp = std::to_string(NUM_SWITCHES - 1) + " UP:";
        ASSERT_STR_CONTAINS(output, lastKeyDown, "Should show last key down");
        ASSERT_STR_CONTAINS(output, lastKeyUp, "Should show last key up");
    }
    else if (test.input.find("SHOW 99") != std::string::npos) {
        // ENHANCED: This will now show exactly what error message is produced vs expected
        std::string expectedError = "Invalid key 0-" + std::to_string(NUM_SWITCHES - 1);
        ASSERT_STR_CONTAINS(output, expectedError, "Should show error for invalid key");
    }
}

//==============================================================================
// MAP COMMAND TESTS
//==============================================================================

void testMapCommand(const TestCase& test) {
    setupTestEnvironment();
    Serial.clear();
    
    processCommand(test.input.c_str());
    
    std::string output = Serial.getFullOutput();
    
    if (test.expected == "OK") {
        ASSERT_STR_CONTAINS(output, "OK", "MAP command should succeed with OK");
        
        // Verify the macro was actually set by checking with SHOW
        if (test.input.find("MAP 0") != std::string::npos) {
            Serial.clear();
            processCommand("SHOW 0");
            ASSERT_TRUE(Serial.hasOutput(), "SHOW should produce output after MAP");
        }
    }
    else if (test.expected == "ERROR") {
        // More specific error checking
        bool hasError = output.find("Invalid key") != std::string::npos || 
                       output.find("Parse error") != std::string::npos ||
                       output.find("error") != std::string::npos;
        ASSERT_TRUE(hasError, "Should show some kind of error message");
        
        // DEBUG: Show what error we actually got
        if (!hasError) {
            std::cout << "Expected error but got: '" << output << "'" << std::endl;
        }
    }
}

//==============================================================================
// CLEAR COMMAND TESTS
//==============================================================================

void testClearCommand(const TestCase& test) {
    setupTestEnvironment();
    
    // Set a macro first
    setTestMacro(0, "test-macro", "test-up");
    
    Serial.clear();
    processCommand(test.input.c_str());
    
    std::string output = Serial.getFullOutput();
    
    if (test.expected == "CLEARED") {
        ASSERT_STR_CONTAINS(output, "Cleared", "CLEAR should show Cleared message");
        
        // Verify macro was actually cleared
        Serial.clear();
        processCommand("SHOW 0");
        std::string showOutput = Serial.getFullOutput();
        ASSERT_STR_CONTAINS(showOutput, "(empty)", "Macro should be empty after clear");
    }
    else if (test.expected == "ERROR") {
        std::string expectedError = "Invalid key 0-" + std::to_string(NUM_SWITCHES - 1);
        ASSERT_STR_CONTAINS(output, expectedError, "Should show invalid key error");
    }
}

//==============================================================================
// STORAGE COMMAND TESTS
//==============================================================================

void testSaveCommand(const TestCase& test) {
    setupTestEnvironment();
    
    // Set some test data
    setTestMacro(0, "save-test");
    
    Serial.clear();
    processCommand("SAVE");
    
    std::string output = Serial.getFullOutput();
    ASSERT_STR_CONTAINS(output, "Saved", "SAVE should show Saved message");
}

void testLoadCommand(const TestCase& test) {
    setupTestEnvironment();
    
    // Set and save some test data first
    setTestMacro(0, "load-test");
    saveToStorage();
    
    // Clear macros and load
    for (int i = 0; i < NUM_SWITCHES; i++) {
        if (macros[i].downMacro) {
            free(macros[i].downMacro);
            macros[i].downMacro = nullptr;
        }
    }
    
    Serial.clear();
    processCommand("LOAD");
    
    std::string output = Serial.getFullOutput();
    ASSERT_STR_CONTAINS(output, "Loaded", "LOAD should show Loaded message");
    
    // Verify data was actually loaded
    Serial.clear();
    processCommand("SHOW 0");
    std::string showOutput = Serial.getFullOutput();
    ASSERT_STR_CONTAINS(showOutput, "load-test", "Loaded macro should be visible");
}

//==============================================================================
// STAT COMMAND TESTS
//==============================================================================

void testStatCommand(const TestCase& test) {
    setupTestEnvironment();
    Serial.clear();
    
    processCommand("STAT");
    
    std::string output = Serial.getFullOutput();
    ASSERT_STR_CONTAINS(output, "Switches:", "STAT should show switches state");
    ASSERT_STR_CONTAINS(output, "Free RAM:", "STAT should show RAM info");
    ASSERT_STR_CONTAINS(output, "0x", "Should show hex switch state");
}

//==============================================================================
// ERROR HANDLING TESTS
//==============================================================================

void testErrorHandling(const TestCase& test) {
    setupTestEnvironment();
    Serial.clear();
    
    processCommand(test.input.c_str());
    
    std::string output = Serial.getFullOutput();
    
    if (test.expected == "UNKNOWN_COMMAND") {
        ASSERT_STR_CONTAINS(output, "Unknown command", "Should show unknown command error");
        ASSERT_STR_CONTAINS(output, "type HELP", "Should suggest HELP");
    }
    else if (test.expected == "INVALID_KEY") {
        std::string expectedError = "Invalid key 0-" + std::to_string(NUM_SWITCHES - 1);
        ASSERT_STR_CONTAINS(output, expectedError, "Should show invalid key error");
    }
    else if (test.expected == "PARSE_ERROR") {
        // More flexible parse error checking
        bool hasParseError = output.find("Parse error") != std::string::npos || 
                           output.find("error") != std::string::npos;
        ASSERT_TRUE(hasParseError, "Should show parse error");
        
        // DEBUG: Show what we actually got if no parse error
        if (!hasParseError) {
            std::cout << "Expected parse error but got: '" << output << "'" << std::endl;
        }
    }
}

//==============================================================================
// INTEGRATION TESTS
//==============================================================================

void testCompleteWorkflow(const TestCase& test) {
    setupTestEnvironment();
    
    // Test a complete workflow: MAP -> SHOW -> SAVE -> CLEAR -> LOAD -> SHOW
    Serial.clear();
    
    // Step 1: Set a macro
    processCommand("MAP 5 \"workflow test\"");
    std::string mapOutput = Serial.getFullOutput();
    ASSERT_STR_CONTAINS(mapOutput, "OK", "MAP should succeed");
    
    // Step 2: Verify it's set
    Serial.clear();
    processCommand("SHOW 5");
    std::string showOutput1 = Serial.getFullOutput();
    ASSERT_STR_CONTAINS(showOutput1, "workflow test", "SHOW should display set macro");
    
    // Step 3: Save to EEPROM
    Serial.clear();
    processCommand("SAVE");
    std::string saveOutput = Serial.getFullOutput();
    ASSERT_STR_CONTAINS(saveOutput, "Saved", "SAVE should succeed");
    
    // Step 4: Clear the macro
    Serial.clear();
    processCommand("CLEAR 5");
    std::string clearOutput = Serial.getFullOutput();
    ASSERT_STR_CONTAINS(clearOutput, "Cleared", "CLEAR should succeed");
    
    // Step 5: Verify it's cleared
    Serial.clear();
    processCommand("SHOW 5");
    std::string showOutput2 = Serial.getFullOutput();
    ASSERT_STR_CONTAINS(showOutput2, "(empty)", "SHOW should show empty after clear");
    
    // Step 6: Load from EEPROM
    Serial.clear();
    processCommand("LOAD");
    std::string loadOutput = Serial.getFullOutput();
    ASSERT_STR_CONTAINS(loadOutput, "Loaded", "LOAD should succeed");
    
    // Step 7: Verify it's restored
    Serial.clear();
    processCommand("SHOW 5");
    std::string showOutput3 = Serial.getFullOutput();
    ASSERT_STR_CONTAINS(showOutput3, "workflow test", "SHOW should display restored macro");
}

//==============================================================================
// DEBUG HELPER FUNCTION
//==============================================================================

void debugCurrentImplementation() {
    std::cout << "\n=== DEBUGGING CURRENT IMPLEMENTATION ===" << std::endl;
    std::cout << "NUM_SWITCHES = " << NUM_SWITCHES << std::endl;
    std::cout << "Expected key range: 0-" << (NUM_SWITCHES - 1) << std::endl;
    
    // Test HELP command output
    setupTestEnvironment();
    Serial.clear();
    processCommand("HELP");
    std::string helpOutput = Serial.getFullOutput();
    
    std::cout << "\nHELP command output:" << std::endl;
    std::cout << "'" << helpOutput << "'" << std::endl;
    
    // Look for key range patterns
    if (helpOutput.find("Keys: 0-23") != std::string::npos) {
        std::cout << "❌ Found hardcoded 'Keys: 0-23'" << std::endl;
    }
    if (helpOutput.find("Keys: 0-" + std::to_string(NUM_SWITCHES - 1)) != std::string::npos) {
        std::cout << "✅ Found correct key range" << std::endl;
    }
    
    // Test invalid key error
    Serial.clear();
    processCommand("SHOW 99");
    std::string errorOutput = Serial.getFullOutput();
    
    std::cout << "\nInvalid key error output:" << std::endl;
    std::cout << "'" << errorOutput << "'" << std::endl;
    
    if (errorOutput.find("Invalid key 0-23") != std::string::npos) {
        std::cout << "❌ Found hardcoded 'Invalid key 0-23'" << std::endl;
    }
    if (errorOutput.find("Invalid key 0-" + std::to_string(NUM_SWITCHES - 1)) != std::string::npos) {
        std::cout << "✅ Found correct error message" << std::endl;
    }
    
    std::cout << "========================================\n" << std::endl;
}

//==============================================================================
// TEST CASE DEFINITIONS
//==============================================================================

std::vector<TestCase> createCommandParsingTests() {
    return {
        TestCase("HELP command", "HELP", "RECOGNIZED"),
        TestCase("SHOW command", "SHOW 0", "RECOGNIZED"),
        TestCase("MAP command", "MAP 0 \"test\"", "RECOGNIZED"),
        TestCase("CLEAR command", "CLEAR 0", "RECOGNIZED"),
        TestCase("LOAD command", "LOAD", "RECOGNIZED"),
        TestCase("SAVE command", "SAVE", "RECOGNIZED"),
        TestCase("STAT command", "STAT", "RECOGNIZED"),
        TestCase("Unknown command", "BADCMD", "UNKNOWN"),
        TestCase("Empty command", "", "RECOGNIZED"),
    };
}

std::vector<TestCase> createShowCommandTests() {
    return {
        TestCase("Show key 0 down", "SHOW 0", "SHOW_DOWN"),
        TestCase("Show key 1 up", "SHOW 1 UP", "SHOW_UP"),
        TestCase("Show key 5 up only", "SHOW 5 UP", "SHOW_UP_ONLY"),
        TestCase("Show empty key", "SHOW 2", "SHOW_EMPTY"),
        TestCase("Show all keys", "SHOW ALL", "SHOW_ALL"),
        TestCase("Show invalid key", "SHOW 99", "INVALID_KEY"),
    };
}

std::vector<TestCase> createMapCommandTests() {
    return {
        TestCase("Simple MAP", "MAP 0 \"hello\"", "OK"),
        TestCase("MAP with modifier", "MAP 1 CTRL C", "OK"),
        TestCase("MAP up direction", "MAP 2 up \"up-test\"", "OK"),
        TestCase("MAP down direction", "MAP 3 down \"down-test\"", "OK"),
        TestCase("MAP invalid key", "MAP 99 \"test\"", "ERROR"),
        TestCase("MAP parse error", "MAP 0 UNKNOWN_KEY", "ERROR"),
    };
}

std::vector<TestCase> createClearCommandTests() {
    return {
        TestCase("Clear down macro", "CLEAR 0", "CLEARED"),
        TestCase("Clear up macro", "CLEAR 0 UP", "CLEARED"),
        TestCase("Clear invalid key", "CLEAR 99", "ERROR"),
    };
}

std::vector<TestCase> createErrorHandlingTests() {
    return {
        TestCase("Unknown command", "BADCOMMAND", "UNKNOWN_COMMAND"),
        TestCase("Invalid key in SHOW", "SHOW 99", "INVALID_KEY"),
        TestCase("Invalid key in MAP", "MAP 99 \"test\"", "INVALID_KEY"),
        TestCase("Invalid key in CLEAR", "CLEAR 99", "INVALID_KEY"),
        TestCase("Parse error in MAP", "MAP 0 BADKEY", "PARSE_ERROR"),
    };
}

//==============================================================================
// MAIN TEST RUNNER
//==============================================================================

int main(int argc, char* argv[]) {
    bool verbose = (argc > 1 && strcmp(argv[1], "-v") == 0);
    bool debug = (argc > 1 && strcmp(argv[1], "-d") == 0);
    
    if (debug) {
        debugCurrentImplementation();
        return 0;
    }
    
    std::cout << "Running Serial Interface Tests (NUM_SWITCHES=" << NUM_SWITCHES << ")" << std::endl;
    std::cout << "Enhanced with ASSERT_STR_CONTAINS for better debugging" << std::endl;
    std::cout << "=============================================================" << std::endl << std::endl;
    
    TestRunner runner(verbose);
    
    std::cout << "Command Parsing Tests:" << std::endl;
    auto parsingTests = createCommandParsingTests();
    for (const auto& test : parsingTests) {
        runner.runTest(test, testCommandParsing);
    }
    
    std::cout << std::endl << "HELP Command Tests:" << std::endl;
    TestCase helpTest("HELP output", "HELP", "HELP_OUTPUT");
    runner.runTest(helpTest, testHelpCommand);
    
    std::cout << std::endl << "SHOW Command Tests:" << std::endl;
    auto showTests = createShowCommandTests();
    for (const auto& test : showTests) {
        runner.runTest(test, testShowCommand);
    }
    
    std::cout << std::endl << "MAP Command Tests:" << std::endl;
    auto mapTests = createMapCommandTests();
    for (const auto& test : mapTests) {
        runner.runTest(test, testMapCommand);
    }
    
    std::cout << std::endl << "CLEAR Command Tests:" << std::endl;
    auto clearTests = createClearCommandTests();
    for (const auto& test : clearTests) {
        runner.runTest(test, testClearCommand);
    }
    
    std::cout << std::endl << "Storage Command Tests:" << std::endl;
    TestCase saveTest("SAVE command", "SAVE", "SAVED");
    TestCase loadTest("LOAD command", "LOAD", "LOADED");
    runner.runTest(saveTest, testSaveCommand);
    runner.runTest(loadTest, testLoadCommand);
    
    std::cout << std::endl << "STAT Command Tests:" << std::endl;
    TestCase statTest("STAT command", "STAT", "STAT_OUTPUT");
    runner.runTest(statTest, testStatCommand);
    
    std::cout << std::endl << "Error Handling Tests:" << std::endl;
    auto errorTests = createErrorHandlingTests();
    for (const auto& test : errorTests) {
        runner.runTest(test, testErrorHandling);
    }
    
    std::cout << std::endl << "Integration Tests:" << std::endl;
    TestCase workflowTest("Complete workflow", "WORKFLOW", "WORKFLOW_SUCCESS");
    runner.runTest(workflowTest, testCompleteWorkflow);
    
    std::cout << std::endl;
    runner.printSummary();
    
    if (!runner.allPassed()) {
        std::cout << std::endl << "💡 TIP: Run with -d flag to see debug output:" << std::endl;
        std::cout << "./test-serial -d" << std::endl;
    }
    
    return runner.allPassed() ? 0 : 1;
}
