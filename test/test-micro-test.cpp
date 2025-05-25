/*
 * Test Framework Verification
 * Simple test to verify the micro-test framework works correctly
 */

#include "Arduino.h"
#include "micro-test.h"
#include <iostream>

//==============================================================================
// SIMPLE TEST FUNCTION
//==============================================================================

std::pair<std::string, std::string> simpleTransform(const std::string& input) {
    // Simple transformation: add "transformed_" prefix
    return std::make_pair("transformed_" + input, "");
}

std::pair<std::string, std::string> errorFunction(const std::string& input) {
    // Always return an error
    return std::make_pair("", "Test error message");
}

//==============================================================================
// FRAMEWORK TEST CASES
//==============================================================================

int main() {
    TestRunner runner(true); // Verbose mode
    
    std::cout << "Testing Micro-Test Framework\n";
    std::cout << "============================\n\n";
    
    // Test 1: Input equals expected (should pass)
    TestCase test1("Same input/output", "hello", "hello");
    runner.runRoundTripTest(test1, [](const std::string& input) {
        return std::make_pair(input, ""); // Identity function
    });
    
    // Test 2: Input different from expected (should pass)
    TestCase test2("Different input/output", "hello", "transformed_hello");
    runner.runRoundTripTest(test2, simpleTransform);
    
    // Test 3: Expected failure (should pass)
    TestCase test3("Expected error", "anything", "", false);
    runner.runRoundTripTest(test3, errorFunction);
    
    // Test 4: Unexpected failure (should fail)
    TestCase test4("Unexpected error", "hello", "hello", true);
    runner.runRoundTripTest(test4, errorFunction);
    
    // Test 5: Wrong output (should fail)
    TestCase test5("Wrong output", "hello", "goodbye");
    runner.runRoundTripTest(test5, [](const std::string& input) {
        return std::make_pair("hello", ""); // Always return "hello"
    });
    
    std::cout << "\n";
    runner.printSummary();
    
    std::cout << "\nExpected: 3/5 tests passed (2 failed)\n";
    
    return 0;
}
