/*
 * Micro Test Framework
 * Minimal testing utilities for C++ without external dependencies
 */

#ifndef MICRO_TEST_H
#define MICRO_TEST_H

#include <iostream>
#include <string>
#include <vector>
#include <functional>

//==============================================================================
// TEST EXPECTATION CONSTANTS
//==============================================================================

static const std::string EXPECT_FAIL = "__EXPECT_FAIL__";
static const std::string EXPECT_PASS = "__EXPECT_PASS__";

//==============================================================================
// TEST CASE STRUCTURE
//==============================================================================

struct TestCase {
    std::string name;
    std::string input;
    std::string expected;
    bool shouldSucceed;
    
    // Single constructor that handles both cases
    TestCase(const std::string& n, const std::string& i, const std::string& third_param)
        : name(n), input(i) {
        
        if (third_param == EXPECT_FAIL) {
            // This is an error test case
            expected = "";
            shouldSucceed = false;
        } else if (third_param == EXPECT_PASS) {
            // This is a pass test case (no expected output)
            expected = "";
            shouldSucceed = true;
        } else {
            // This is a normal test case with expected output
            expected = third_param;
            shouldSucceed = true;
        }
    }
};

//==============================================================================
// TEST RUNNER CLASS
//==============================================================================

class TestRunner {
private:
    int totalTests = 0;
    int passedTests = 0;
    bool verbose = false;
    
    void printTestHeader(const TestCase& test) {
        if (verbose) {
            std::cout << "Test: " << test.name << std::endl;
            std::cout << "  Input: '" << test.input << "'" << std::endl;
        }
    }
    
    void printSuccess(const std::string& message = "") {
        if (verbose) {
            std::cout << "  ✓ " << (message.empty() ? "PASS" : message) << std::endl;
        }
        passedTests++;
    }
    
    void printFailure(const std::string& message) {
        std::cout << "FAIL: " << message << std::endl;
    }
    
    void printBytes(const uint8_t* bytes, size_t length) {
        if (verbose && bytes && length > 0) {
            std::cout << "  Bytes: ";
            for (size_t i = 0; i < length; i++) {
                printf("0x%02X ", bytes[i]);
            }
            std::cout << std::endl;
        }
    }
    
public:
    TestRunner(bool v = false) : verbose(v) {}
    
    void setVerbose(bool v) { verbose = v; }
    
    // Exception-based test runner
    void runTest(const TestCase& test, std::function<void(const TestCase&)> testFunc) {
        totalTests++;
        printTestHeader(test);
        
        try {
            testFunc(test);
            
            // If we get here and shouldSucceed is false, that's wrong
            if (!test.shouldSucceed) {
                printFailure("Expected failure but test succeeded");
            } else {
                printSuccess("Test passed");
            }
        } catch (const std::exception& e) {
            if (!test.shouldSucceed) {
                printSuccess("Expected failure: " + std::string(e.what()));
            } else {
                printFailure(std::string(e.what()));
            }
        } catch (...) {
            printFailure("Unknown exception");
        }
        
        if (verbose) std::cout << std::endl;
    }
    
    // Helper for binary data display
    void showBytes(const uint8_t* bytes, size_t length) {
        printBytes(bytes, length);
    }
    
    void printSummary() {
        std::cout << passedTests << "/" << totalTests << " tests passed";
        if (passedTests != totalTests) {
            std::cout << " (" << (totalTests - passedTests) << " failed)";
        }
        std::cout << std::endl;
        
        std::cout << std::endl << "Expected: " << passedTests << "/" << totalTests << " tests passed";
        if (passedTests != totalTests) {
            std::cout << " (" << (totalTests - passedTests) << " failed)";
        }
        std::cout << std::endl;
    }
    
    bool allPassed() const {
        return passedTests == totalTests;
    }
    
    int getPassedCount() const { return passedTests; }
    int getTotalCount() const { return totalTests; }
};

//==============================================================================
// ASSERTION MACROS
//==============================================================================

#define ASSERT_TRUE(condition, message) \
    do { \
        if (!(condition)) { \
            throw std::runtime_error(std::string("Assertion failed: ") + (message)); \
        } \
    } while(0)

#define ASSERT_EQ(actual, expected, message) \
    do { \
        if ((actual) != (expected)) { \
            throw std::runtime_error(std::string("Assertion failed: ") + (message) + \
                                   " (got: " + std::to_string(actual) + \
                                   ", expected: " + std::to_string(expected) + ")"); \
        } \
    } while(0)

#define ASSERT_STR_EQ(actual, expected, message) \
    do { \
        if (std::string(actual) != std::string(expected)) { \
            throw std::runtime_error(std::string("Assertion failed: ") + (message) + \
                                   " (got: '" + std::string(actual) + \
                                   "', expected: '" + std::string(expected) + "')"); \
        } \
    } while(0)

#endif // MICRO_TEST_H