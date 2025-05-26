/*
 * Test Framework Verification
 * Simple test to verify the micro-test framework works correctly
 */

#include "Arduino.h"
#include "micro-test.h"
#include <iostream>

//==============================================================================
// SIMPLE TEST FUNCTIONS
//==============================================================================

std::pair<std::string, std::string> simpleTransform(const std::string& input) {
    // Simple transformation: add "transformed_" prefix
    return std::make_pair("transformed_" + input, "");
}

std::pair<std::string, std::string> errorFunction(const std::string& input) {
    // Always return an error
    return std::make_pair("", "Test error message");
}

std::pair<std::string, std::string> identityFunction(const std::string& input) {
    // Identity function - returns input unchanged
    return std::make_pair(input, "");
}

//==============================================================================
// TEST FUNCTIONS USING NEW INTERFACE
//==============================================================================

void testIdentityFunction(const TestCase& test) {
    auto result = identityFunction(test.input);
    
    if (!result.second.empty()) {
        ASSERT_FAIL("Unexpected error: " + result.second);
    }
    
    ASSERT_STR_EQ(result.first, test.expected, "Output should match expected");
}

void testTransformFunction(const TestCase& test) {
    auto result = simpleTransform(test.input);
    
    if (!result.second.empty()) {
        ASSERT_FAIL("Unexpected error: " + result.second);
    }
    
    ASSERT_STR_EQ(result.first, test.expected, "Transform output should match expected");
}

void testExpectedFailure(const TestCase& test) {
    // For expected failure tests, we need to throw an exception
    // This simulates a test that should fail
    auto result = errorFunction(test.input);
    
    if (!result.second.empty()) {
        // Function returned an error - throw exception to simulate test failure
        throw std::runtime_error("Expected failure: " + result.second);
    } else {
        // Function succeeded when we expected it to fail
        ASSERT_FAIL("Expected function to fail but it succeeded");
    }
}

void testSuccessfulFunction(const TestCase& test) {
    auto result = identityFunction(test.input);
    
    if (!result.second.empty()) {
        ASSERT_FAIL("Unexpected error: " + result.second);
    }
    
    ASSERT_STR_EQ(result.first, test.expected, "Output should match expected");
}

void testWrongOutput(const TestCase& test) {
    // This function always returns "hello" regardless of input
    std::string actualOutput = "hello";
    
    ASSERT_STR_EQ(actualOutput, test.expected, "Output should match expected");
}

void testWrongOutputExpected(const TestCase& test) {
    // This test is designed to fail due to wrong output, but we'll catch and verify
    try {
        testWrongOutput(test);
        ASSERT_FAIL("Expected this test to fail due to wrong output but it passed");
    } catch (const std::exception& e) {
        // Expected failure occurred - this is actually success
        std::string errorMsg = e.what();
        ASSERT_TRUE(errorMsg.find("Output should match expected") != std::string::npos, 
                   "Error message should indicate output mismatch");
        // Test passes because we caught the expected assertion failure
    }
}

//==============================================================================
// MAIN TEST RUNNER
//==============================================================================

int main() {
    TestRunner runner(true); // Verbose mode
    
    std::cout << "Testing Micro-Test Framework\n";
    std::cout << "============================\n\n";
    
    // Test 1: Input equals expected (should pass)
    TestCase test1("Same input/output", "hello", "hello");
    runner.runTest(test1, testIdentityFunction);
    
    // Test 2: Input different from expected (should pass)
    TestCase test2("Different input/output", "hello", "transformed_hello");
    runner.runTest(test2, testTransformFunction);
    
    // Test 3: Expected failure (should pass by handling the expected failure correctly)
    TestCase test3("Expected error", "anything", EXPECT_FAIL);
    runner.runTest(test3, testExpectedFailure);
    
    // Test 4: Function that should succeed (should pass)
    TestCase test4("Function produces expected error", "hello", "hello");
    runner.runTest(test4, testSuccessfulFunction);
    
    // Test 5: Wrong output (should pass by catching and verifying the assertion failure)
    TestCase test5("Framework catches wrong output", "hello", "goodbye");
    runner.runTest(test5, testWrongOutputExpected);
    
    std::cout << "\n";
    runner.printSummary();
    
    std::cout << "\nAll tests should now pass - framework correctly detects and handles failures\n";
    
    return runner.allPassed() ? 0 : 1;
}