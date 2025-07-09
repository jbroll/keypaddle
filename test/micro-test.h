/*
 * Micro Test Framework - Enhanced with String Contains Assertion
 * Minimal testing utilities for C++ without external dependencies
 */

#ifndef MICRO_TEST_H
#define MICRO_TEST_H

#include <iostream>
#include <string>
#include <vector>
#include <functional>
#include <sstream>

//==============================================================================
// UTILITY FUNCTIONS
//==============================================================================

// Extract just the filename from a full path
inline std::string getFileName(const std::string& path) {
    size_t pos = path.find_last_of("/\\");
    return (pos == std::string::npos) ? path : path.substr(pos + 1);
}

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
                printFailure(test.name + ": Expected failure but test succeeded");
            } else {
                printSuccess("Test passed");
            }
        } catch (const std::exception& e) {
            if (!test.shouldSucceed) {
                printSuccess("Expected failure: " + std::string(e.what()));
            } else {
                // Re-throw with test name prepended for better error tracking
                std::string enhancedMessage = test.name + ": " + std::string(e.what());
                printFailure(enhancedMessage);
            }
        } catch (...) {
            printFailure(test.name + ": Unknown exception");
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

#define ASSERT_FAIL(msg) \
    do { \
        std::ostringstream oss; \
        oss << "at " << getFileName(__FILE__) << ":" << __LINE__ << ": " << (msg); \
        throw std::runtime_error(oss.str()); \
    } while(0)

#define ASSERT_TRUE(condition, message) \
    do { \
        if (!(condition)) { \
            std::ostringstream oss; \
            oss << "at " << getFileName(__FILE__) << ":" << __LINE__ << ": " \
                << "Assertion failed: " << (message) \
                << " (expected true but got false)"; \
            throw std::runtime_error(oss.str()); \
        } \
    } while(0)

#define ASSERT_FALSE(condition, message) \
    do { \
        if (condition) { \
            std::ostringstream oss; \
            oss << "at " << getFileName(__FILE__) << ":" << __LINE__ << ": " \
                << "Assertion failed: " << (message) \
                << " (expected false but got true)"; \
            throw std::runtime_error(oss.str()); \
        } \
    } while(0)

#define ASSERT_EQ(actual, expected, message) \
    do { \
        if ((actual) != (expected)) { \
            std::ostringstream oss; \
            oss << "at " << getFileName(__FILE__) << ":" << __LINE__ << ": " \
                << "Assertion failed: " << (message) \
                << " (got: " << (actual) << ", expected: " << (expected) << ")"; \
            throw std::runtime_error(oss.str()); \
        } \
    } while(0)

#define ASSERT_STR_EQ(actual, expected, message) \
    do { \
        if (std::string(actual) != std::string(expected)) { \
            std::ostringstream oss; \
            oss << "at " << getFileName(__FILE__) << ":" << __LINE__ << ": " \
                << "Assertion failed: " << (message) \
                << " (got: '" << std::string(actual) \
                << "', expected: '" << std::string(expected) << "')"; \
            throw std::runtime_error(oss.str()); \
        } \
    } while(0)

#define ASSERT_STR_CONTAINS(haystack, needle, message) \
    do { \
        std::string _haystack_str = std::string(haystack); \
        std::string _needle_str = std::string(needle); \
        if (_haystack_str.find(_needle_str) == std::string::npos) { \
            std::ostringstream oss; \
            oss << "at " << getFileName(__FILE__) << ":" << __LINE__ << ": " \
                << "Assertion failed: " << (message) \
                << " (looking for: '" << _needle_str \
                << "', in: '" << _haystack_str << "')"; \
            throw std::runtime_error(oss.str()); \
        } \
    } while(0)

#define ASSERT_STR_NOT_CONTAINS(haystack, needle, message) \
    do { \
        std::string _haystack_str = std::string(haystack); \
        std::string _needle_str = std::string(needle); \
        if (_haystack_str.find(_needle_str) != std::string::npos) { \
            std::ostringstream oss; \
            oss << "at " << getFileName(__FILE__) << ":" << __LINE__ << ": " \
                << "Assertion failed: " << (message) \
                << " (should not contain: '" << _needle_str \
                << "', but found in: '" << _haystack_str << "')"; \
            throw std::runtime_error(oss.str()); \
        } \
    } while(0)

#endif // MICRO_TEST_H
