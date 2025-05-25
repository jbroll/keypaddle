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
// TEST CASE STRUCTURE
//==============================================================================

struct TestCase {
    std::string name;
    std::string input;
    std::string expectedOutput;  // Empty means expect same as input
    bool shouldSucceed;
    
    TestCase(const std::string& n, const std::string& i, bool succeed = true)
        : name(n), input(i), expectedOutput(""), shouldSucceed(succeed) {}
        
    TestCase(const std::string& n, const std::string& i, const std::string& expected, bool succeed = true)
        : name(n), input(i), expectedOutput(expected), shouldSucceed(succeed) {}
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
    
    // Generic test runner for any test function
    void runTest(const TestCase& test, std::function<bool(const TestCase&)> testFunc) {
        totalTests++;
        printTestHeader(test);
        
        try {
            if (testFunc(test)) {
                printSuccess();
            }
        } catch (const std::exception& e) {
            printFailure(std::string("Exception: ") + e.what());
        } catch (...) {
            printFailure("Unknown exception");
        }
        
        if (verbose) std::cout << std::endl;
    }
    
    // Specialized for round-trip testing
    void runRoundTripTest(const TestCase& test,
                         std::function<std::pair<std::string, std::string>(const std::string&)> roundTripFunc) {
        totalTests++;
        printTestHeader(test);
        
        try {
            std::pair<std::string, std::string> result_pair = roundTripFunc(test.input);
            std::string result = result_pair.first;
            std::string error = result_pair.second;
            
            if (!test.shouldSucceed) {
                if (!error.empty()) {
                    printSuccess("Expected failure: " + error);
                } else {
                    printFailure("Expected failure but test succeeded");
                }
                return;
            }
            
            if (!error.empty()) {
                printFailure("Unexpected error: " + error);
                return;
            }
            
            std::string expected = test.expectedOutput.empty() ? test.input : test.expectedOutput;
            
            if (verbose) {
                std::cout << "  Decoded: '" << result << "'" << std::endl;
                std::cout << "  Expected: '" << expected << "'" << std::endl;
            }
            
            if (result == expected) {
                printSuccess("Round-trip successful");
            } else {
                printFailure("Round-trip failed - output differs");
                if (!verbose) {
                    std::cout << "    Got: '" << result << "'" << std::endl;
                    std::cout << "    Expected: '" << expected << "'" << std::endl;
                }
            }
        } catch (const std::exception& e) {
            printFailure(std::string("Exception: ") + e.what());
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