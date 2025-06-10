/*
 * Storage System Testing
 * Tests EEPROM save/load functionality with the macro storage system
 */

#include "Arduino.h"
#include "EEPROM.h"
#include "micro-test.h"

// Include the actual implementation files from parent directory
#include "../config.h"
#include "../storage.h"
#include "../macro-encode.h"

#include <iostream>
#include <cstring>

//==============================================================================
// TEST HELPER FUNCTIONS
//==============================================================================

void clearAllMacros() {
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

bool compareMacros(int keyIndex, const char* expectedDown, const char* expectedUp = nullptr) {
    if (keyIndex < 0 || keyIndex >= NUM_SWITCHES) return false;
    
    // Compare down macro
    if (expectedDown == nullptr || strlen(expectedDown) == 0) {
        if (macros[keyIndex].downMacro != nullptr) return false;
    } else {
        if (macros[keyIndex].downMacro == nullptr) return false;
        if (strcmp(macros[keyIndex].downMacro, expectedDown) != 0) return false;
    }
    
    // Compare up macro
    if (expectedUp == nullptr || strlen(expectedUp) == 0) {
        if (macros[keyIndex].upMacro != nullptr) return false;
    } else {
        if (macros[keyIndex].upMacro == nullptr) return false;
        if (strcmp(macros[keyIndex].upMacro, expectedUp) != 0) return false;
    }
    
    return true;
}

std::string macroToString(const char* macro) {
    return macro ? std::string("\"") + macro + "\"" : "null";
}

void printMacroState(int keyIndex) {
    if (keyIndex < 0 || keyIndex >= NUM_SWITCHES) return;
    
    std::cout << "  Key " << keyIndex << ": down=" 
              << macroToString(macros[keyIndex].downMacro)
              << ", up=" << macroToString(macros[keyIndex].upMacro) << std::endl;
}

//==============================================================================
// BASIC STORAGE TESTS
//==============================================================================

void testEmptyStorageLoad(const TestCase& test) {
    // Clear EEPROM and macros
    EEPROM.clear();
    clearAllMacros();
    setupStorage();
    
    // Try to load from empty EEPROM
    bool result = loadFromStorage();
    
    // Should return false (no valid data)
    ASSERT_TRUE(result == false, "loadFromStorage should return false for empty EEPROM");
    
    // All macros should remain null
    for (int i = 0; i < NUM_SWITCHES; i++) {
        ASSERT_TRUE(macros[i].downMacro == nullptr, "Down macro should be null after failed load");
        ASSERT_TRUE(macros[i].upMacro == nullptr, "Up macro should be null after failed load");
    }
}

void testBasicSaveLoad(const TestCase& test) {
    // Clear EEPROM and set up test data
    EEPROM.clear();
    clearAllMacros();
    setupStorage();
    
    // Set some test macros
    setTestMacro(0, "hello");
    setTestMacro(1, "world", "up-world");
    setTestMacro(5, "", "just-up");  // Empty down, non-empty up
    setTestMacro(10, "just-down", ""); // Non-empty down, empty up
    
    // Save to EEPROM
    bool saveResult = saveToStorage();
    ASSERT_TRUE(saveResult, "saveToStorage should succeed");
    
    // Clear macros in memory
    clearAllMacros();
    
    // Load from EEPROM
    bool loadResult = loadFromStorage();
    ASSERT_TRUE(loadResult, "loadFromStorage should succeed");
    
    // Verify loaded data
    ASSERT_TRUE(compareMacros(0, "hello", nullptr), "Key 0 should match saved data");
    ASSERT_TRUE(compareMacros(1, "world", "up-world"), "Key 1 should match saved data");
    ASSERT_TRUE(compareMacros(5, nullptr, "just-up"), "Key 5 should match saved data");
    ASSERT_TRUE(compareMacros(10, "just-down", nullptr), "Key 10 should match saved data");
    
    // Check that other keys are empty
    ASSERT_TRUE(compareMacros(2, nullptr, nullptr), "Key 2 should be empty");
    ASSERT_TRUE(compareMacros(23, nullptr, nullptr), "Key 23 should be empty");
}

void testAllKeysPopulated(const TestCase& test) {
    // Test with all 24 keys having different macros
    EEPROM.clear();
    clearAllMacros();
    setupStorage();
    
    // Set unique macros for all keys
    for (int i = 0; i < NUM_SWITCHES; i++) {
        std::string downMacro = "down" + std::to_string(i);
        std::string upMacro = "up" + std::to_string(i);
        setTestMacro(i, downMacro.c_str(), upMacro.c_str());
    }
    
    // Save and reload
    ASSERT_TRUE(saveToStorage(), "Save should succeed");
    clearAllMacros();
    ASSERT_TRUE(loadFromStorage(), "Load should succeed");
    
    // Verify all keys
    for (int i = 0; i < NUM_SWITCHES; i++) {
        std::string expectedDown = "down" + std::to_string(i);
        std::string expectedUp = "up" + std::to_string(i);
        ASSERT_TRUE(compareMacros(i, expectedDown.c_str(), expectedUp.c_str()), 
                   "Key " + std::to_string(i) + " should match saved data");
    }
}

void testMagicNumberValidation(const TestCase& test) {
    EEPROM.clear();
    clearAllMacros();
    setupStorage();
    
    // Set and save some data
    setTestMacro(0, "test");
    ASSERT_TRUE(saveToStorage(), "Initial save should succeed");
    
    // Corrupt the magic number
    EEPROM.write(0, 0x00);  // First byte of magic number
    
    // Try to load - should fail
    clearAllMacros();
    bool loadResult = loadFromStorage();
    ASSERT_TRUE(loadResult == false, "Load should fail with corrupted magic number");
    
    // Macros should remain empty
    ASSERT_TRUE(compareMacros(0, nullptr, nullptr), "Macro should be empty after failed load");
}

//==============================================================================
// UTF-8+ MACRO INTEGRATION TESTS
//==============================================================================

void testUTF8MacroStorage(const TestCase& test) {
    // Test saving and loading actual UTF-8+ encoded macros
    EEPROM.clear();
    clearAllMacros();
    setupStorage();
    
    // Create some UTF-8+ encoded macros using the encoder
    MacroEncodeResult result1 = macroEncode("CTRL C");
    MacroEncodeResult result2 = macroEncode("\"hello world\"");
    MacroEncodeResult result3 = macroEncode("+SHIFT \"CAPS\" -SHIFT");
    
    ASSERT_TRUE(result1.error == nullptr, "Encode 1 should succeed");
    ASSERT_TRUE(result2.error == nullptr, "Encode 2 should succeed");
    ASSERT_TRUE(result3.error == nullptr, "Encode 3 should succeed");
    
    // Set the encoded macros
    macros[0].downMacro = result1.utf8Sequence;
    macros[1].downMacro = result2.utf8Sequence;
    macros[2].upMacro = result3.utf8Sequence;  // Use as up macro
    
    // Save and reload
    ASSERT_TRUE(saveToStorage(), "Save should succeed");
    
    // Store expected values before clearing
    std::string expected1 = result1.utf8Sequence;
    std::string expected2 = result2.utf8Sequence;
    std::string expected3 = result3.utf8Sequence;
    
    clearAllMacros();
    ASSERT_TRUE(loadFromStorage(), "Load should succeed");
    
    // Verify the UTF-8+ sequences are preserved exactly
    ASSERT_TRUE(compareMacros(0, expected1.c_str(), nullptr), "UTF-8+ macro 1 should be preserved");
    ASSERT_TRUE(compareMacros(1, expected2.c_str(), nullptr), "UTF-8+ macro 2 should be preserved");
    ASSERT_TRUE(compareMacros(2, nullptr, expected3.c_str()), "UTF-8+ macro 3 should be preserved");
}

void testLongMacroStorage(const TestCase& test) {
    // Test with longer macros to verify no truncation
    EEPROM.clear();
    clearAllMacros();
    setupStorage();
    
    // Create a longer macro sequence
    std::string longMacro = "This is a longer macro string with multiple words and punctuation! It should be preserved exactly.";
    std::string longMacro2 = "CTRL A CTRL C ALT TAB \"paste\" ENTER";
    
    // Encode the second macro
    MacroEncodeResult encodedLong = macroEncode(longMacro2.c_str());
    ASSERT_TRUE(encodedLong.error == nullptr, "Long macro encode should succeed");
    
    setTestMacro(0, longMacro.c_str());
    macros[1].downMacro = encodedLong.utf8Sequence;
    
    // Save and reload
    ASSERT_TRUE(saveToStorage(), "Save should succeed");
    
    std::string expectedEncoded = encodedLong.utf8Sequence;
    clearAllMacros();
    ASSERT_TRUE(loadFromStorage(), "Load should succeed");
    
    // Verify long macros are preserved
    ASSERT_TRUE(compareMacros(0, longMacro.c_str(), nullptr), "Long text macro should be preserved");
    ASSERT_TRUE(compareMacros(1, expectedEncoded.c_str(), nullptr), "Long encoded macro should be preserved");
}

//==============================================================================
// EDGE CASE AND ERROR TESTS
//==============================================================================

void testEmptyMacroHandling(const TestCase& test) {
    // Test that empty macros (null and empty strings) are handled correctly
    EEPROM.clear();
    clearAllMacros();
    setupStorage();
    
    // Mix of null, empty, and valid macros
    setTestMacro(0, nullptr, nullptr);          // Both null
    setTestMacro(1, "", "");                    // Both empty strings
    setTestMacro(2, "valid", nullptr);          // Down valid, up null
    setTestMacro(3, nullptr, "valid_up");       // Down null, up valid
    setTestMacro(4, "", "valid_up2");           // Down empty, up valid
    setTestMacro(5, "valid_down2", "");         // Down valid, up empty
    
    // Save and reload
    ASSERT_TRUE(saveToStorage(), "Save should succeed");
    clearAllMacros();
    ASSERT_TRUE(loadFromStorage(), "Load should succeed");
    
    // Verify empty handling
    ASSERT_TRUE(compareMacros(0, nullptr, nullptr), "Null macros should remain null");
    ASSERT_TRUE(compareMacros(1, nullptr, nullptr), "Empty string macros should become null");
    ASSERT_TRUE(compareMacros(2, "valid", nullptr), "Mixed null/valid should work");
    ASSERT_TRUE(compareMacros(3, nullptr, "valid_up"), "Mixed null/valid should work");
    ASSERT_TRUE(compareMacros(4, nullptr, "valid_up2"), "Mixed empty/valid should work");
    ASSERT_TRUE(compareMacros(5, "valid_down2", nullptr), "Mixed valid/empty should work");
}

void testMultipleSaveLoadCycles(const TestCase& test) {
    // Test multiple save/load cycles to ensure consistency
    EEPROM.clear();
    clearAllMacros();
    setupStorage();
    
    for (int cycle = 0; cycle < 3; cycle++) {
        // Set different data each cycle
        std::string data = "cycle" + std::to_string(cycle);
        setTestMacro(cycle, data.c_str());
        setTestMacro(cycle + 10, nullptr, data.c_str());
        
        // Save and reload
        ASSERT_TRUE(saveToStorage(), "Save cycle " + std::to_string(cycle) + " should succeed");
        clearAllMacros();
        ASSERT_TRUE(loadFromStorage(), "Load cycle " + std::to_string(cycle) + " should succeed");
        
        // Verify current cycle data
        ASSERT_TRUE(compareMacros(cycle, data.c_str(), nullptr), "Cycle " + std::to_string(cycle) + " down should match");
        ASSERT_TRUE(compareMacros(cycle + 10, nullptr, data.c_str()), "Cycle " + std::to_string(cycle) + " up should match");
        
        // Verify previous cycles are preserved
        for (int prev = 0; prev < cycle; prev++) {
            std::string prevData = "cycle" + std::to_string(prev);
            ASSERT_TRUE(compareMacros(prev, prevData.c_str(), nullptr), "Previous cycle " + std::to_string(prev) + " should be preserved");
            ASSERT_TRUE(compareMacros(prev + 10, nullptr, prevData.c_str()), "Previous cycle " + std::to_string(prev) + " up should be preserved");
        }
    }
}

//==============================================================================
// TEST CASE DEFINITIONS
//==============================================================================

std::vector<TestCase> createBasicStorageTests() {
    return {
        TestCase("Empty EEPROM load", "", EXPECT_PASS),
        TestCase("Basic save/load", "", EXPECT_PASS),
        TestCase("All keys populated", "", EXPECT_PASS),
        TestCase("Magic number validation", "", EXPECT_PASS),
    };
}

std::vector<TestCase> createUTF8StorageTests() {
    return {
        TestCase("UTF-8+ macro storage", "", EXPECT_PASS),
        TestCase("Long macro storage", "", EXPECT_PASS),
    };
}

std::vector<TestCase> createEdgeCaseTests() {
    return {
        TestCase("Empty macro handling", "", EXPECT_PASS),
        TestCase("Multiple save/load cycles", "", EXPECT_PASS),
    };
}

//==============================================================================
// MAIN TEST RUNNER
//==============================================================================

int main(int argc, char* argv[]) {
    bool verbose = (argc > 1 && strcmp(argv[1], "-v") == 0);
    
    std::cout << "Running Storage System Tests" << std::endl;
    std::cout << "============================" << std::endl << std::endl;
    
    TestRunner runner(verbose);
    
    // Map test functions to test cases
    std::vector<std::pair<TestCase, void(*)(const TestCase&)>> allTests = {
        {TestCase("Empty EEPROM load", "", EXPECT_PASS), testEmptyStorageLoad},
        {TestCase("Basic save/load", "", EXPECT_PASS), testBasicSaveLoad},
        {TestCase("All keys populated", "", EXPECT_PASS), testAllKeysPopulated},
        {TestCase("Magic number validation", "", EXPECT_PASS), testMagicNumberValidation},
        {TestCase("UTF-8+ macro storage", "", EXPECT_PASS), testUTF8MacroStorage},
        {TestCase("Long macro storage", "", EXPECT_PASS), testLongMacroStorage},
        {TestCase("Empty macro handling", "", EXPECT_PASS), testEmptyMacroHandling},
        {TestCase("Multiple save/load cycles", "", EXPECT_PASS), testMultipleSaveLoadCycles},
    };
    
    for (const auto& testPair : allTests) {
        runner.runTest(testPair.first, testPair.second);
    }
    
    std::cout << std::endl;
    runner.printSummary();
    
    if (verbose) {
        std::cout << std::endl << "EEPROM Usage Summary:" << std::endl;
        std::cout << "Total EEPROM size: " << EEPROM.length() << " bytes" << std::endl;
        std::cout << "Used bytes: " << EEPROM.countUsedBytes() << std::endl;
        std::cout << "Magic number location: 0-3" << std::endl;
        std::cout << "Data start: 4" << std::endl;
    }
    
    return runner.allPassed() ? 0 : 1;
}
