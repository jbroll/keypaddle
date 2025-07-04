/*
 * Fixed Chord Storage System Tests
 * FIXED: Properly track modifier mask in verification
 */

#include "Arduino.h"
#include "EEPROM.h"
#include "micro-test.h"

// Include the actual implementation files from parent directory
#include "../config.h"
#include "../storage.h"
#include "../chordStorage.h"
#include "../macro-encode.h"

#include <iostream>
#include <cstring>
#include <vector>
#include <map>

//==============================================================================
// FIXED MOCK CHORD SYSTEM
//==============================================================================

class MockChordSystem {
private:
    std::map<uint32_t, std::string> chordMap;
    uint32_t currentModifierMask;
    int addChordCallCount;
    int clearAllChordsCallCount;
    std::vector<std::string> operationLog;
    
public:
    MockChordSystem() : currentModifierMask(0), addChordCallCount(0), clearAllChordsCallCount(0) {}
    
    void reset() {
        chordMap.clear();
        currentModifierMask = 0;
        addChordCallCount = 0;
        clearAllChordsCallCount = 0;
        operationLog.clear();
        operationLog.push_back("RESET: MockChordSystem cleared");
    }
    
    void setModifierMask(uint32_t mask) {
        currentModifierMask = mask;
        operationLog.push_back("SET_MODIFIER: mask=0x" + std::to_string(mask));
    }
    
    uint32_t getModifierMask() const {
        return currentModifierMask;
    }
    
    bool addChord(uint32_t keyMask, const char* macroSequence) {
        addChordCallCount++;
        if (!macroSequence) {
            operationLog.push_back("ADD_CHORD_FAIL: keyMask=0x" + std::to_string(keyMask) + ", macro=NULL");
            return false;
        }
        chordMap[keyMask] = std::string(macroSequence);
        operationLog.push_back("ADD_CHORD: keyMask=0x" + std::to_string(keyMask) + 
                              ", macro=\"" + std::string(macroSequence) + "\"");
        return true;
    }
    
    void clearAllChords() {
        clearAllChordsCallCount++;
        size_t oldSize = chordMap.size();
        chordMap.clear();
        operationLog.push_back("CLEAR_ALL: removed " + std::to_string(oldSize) + " chords");
    }
    
    void forEachChord(void (*callback)(uint32_t keyMask, const char* macro)) {
        operationLog.push_back("FOR_EACH: iterating " + std::to_string(chordMap.size()) + " chords");
        for (const auto& pair : chordMap) {
            callback(pair.first, pair.second.c_str());
        }
    }
    
    // Test utilities
    size_t getChordCount() const { return chordMap.size(); }
    bool hasChord(uint32_t keyMask) const { return chordMap.find(keyMask) != chordMap.end(); }
    std::string getChordMacro(uint32_t keyMask) const {
        auto it = chordMap.find(keyMask);
        return (it != chordMap.end()) ? it->second : "";
    }
    int getAddChordCallCount() const { return addChordCallCount; }
    int getClearAllChordsCallCount() const { return clearAllChordsCallCount; }
    
    std::string getOperationLog() const {
        std::string result;
        for (size_t i = 0; i < operationLog.size(); i++) {
            if (i > 0) result += "\n";
            result += operationLog[i];
        }
        return result;
    }
    
    std::string getCurrentState() const {
        std::string result = "MockChordSystem{modifierMask=0x" + std::to_string(currentModifierMask);
        result += ", chordCount=" + std::to_string(chordMap.size());
        result += ", addCalls=" + std::to_string(addChordCallCount);
        result += ", clearCalls=" + std::to_string(clearAllChordsCallCount);
        result += "}";
        return result;
    }
};

// Global mock instance
MockChordSystem mockChordSystem;

//==============================================================================
// CALLBACK FUNCTION IMPLEMENTATIONS
//==============================================================================

void mockForEachChord(void (*callback)(uint32_t keyMask, const char* macro)) {
    mockChordSystem.forEachChord(callback);
}

bool mockAddChord(uint32_t keyMask, const char* macroSequence) {
    return mockChordSystem.addChord(keyMask, macroSequence);
}

void mockClearAllChords() {
    mockChordSystem.clearAllChords();
}

//==============================================================================
// TEST HELPER FUNCTIONS
//==============================================================================

void setupTestEnvironment() {
    EEPROM.clear();
    mockChordSystem.reset();
}

void setTestChordState(uint32_t modifierMask, const std::vector<std::pair<uint32_t, std::string>>& chords) {
    mockChordSystem.setModifierMask(modifierMask);
    for (const auto& chord : chords) {
        mockChordSystem.addChord(chord.first, chord.second.c_str());
    }
}

// FIXED: Don't rely on mock system's modifier mask, use the actual return value
bool verifyChordState(uint32_t actualModifierMask, uint32_t expectedModifierMask, 
                     const std::vector<std::pair<uint32_t, std::string>>& expectedChords, 
                     std::string& debugInfo) {
    debugInfo = "VERIFICATION:\n";
    debugInfo += "Expected modifierMask=0x" + std::to_string(expectedModifierMask) + ", chordCount=" + std::to_string(expectedChords.size()) + "\n";
    debugInfo += "Actual: " + mockChordSystem.getCurrentState() + ", returnedModifierMask=0x" + std::to_string(actualModifierMask) + "\n";
    
    // FIXED: Check the actual returned modifier mask, not the mock's internal state
    if (actualModifierMask != expectedModifierMask) {
        debugInfo += "FAIL: Modifier mask mismatch - expected 0x" + 
                    std::to_string(expectedModifierMask) + ", got 0x" + 
                    std::to_string(actualModifierMask) + "\n";
        return false;
    }
    
    // Check chord count
    if (mockChordSystem.getChordCount() != expectedChords.size()) {
        debugInfo += "FAIL: Chord count mismatch - expected " + 
                    std::to_string(expectedChords.size()) + ", got " + 
                    std::to_string(mockChordSystem.getChordCount()) + "\n";
        return false;
    }
    
    // Check each chord
    for (const auto& expectedChord : expectedChords) {
        if (!mockChordSystem.hasChord(expectedChord.first)) {
            debugInfo += "FAIL: Missing chord 0x" + std::to_string(expectedChord.first) + "\n";
            return false;
        }
        
        std::string actualMacro = mockChordSystem.getChordMacro(expectedChord.first);
        if (actualMacro != expectedChord.second) {
            debugInfo += "FAIL: Chord 0x" + std::to_string(expectedChord.first) + 
                        " macro mismatch - expected \"" + expectedChord.second + 
                        "\", got \"" + actualMacro + "\"\n";
            return false;
        }
    }
    
    debugInfo += "SUCCESS: All verifications passed\n";
    return true;
}

std::string encodeTestMacro(const std::string& macroCommand) {
    MacroEncodeResult result = macroEncode(macroCommand.c_str());
    if (result.error != nullptr) {
        return ""; // Encoding failed
    }
    
    std::string encoded = result.utf8Sequence;
    free(result.utf8Sequence);
    return encoded;
}

//==============================================================================
// BASIC SAVE/LOAD TESTS (PASSING)
//==============================================================================

void testEmptyChordStorage(const TestCase& test) {
    setupTestEnvironment();
    
    uint16_t startOffset = 100;
    uint32_t modifierMask = loadChords(startOffset, mockAddChord, mockClearAllChords);
    
    std::string operationLog = mockChordSystem.getOperationLog();
    
    ASSERT_EQ(modifierMask, 0, "loadChords should return 0 for empty EEPROM");
    ASSERT_STR_CONTAINS(operationLog, "CLEAR_ALL", "clearAllChords should be called during load");
    ASSERT_EQ(mockChordSystem.getAddChordCallCount(), 0, "No chords should be added from empty EEPROM");
}

void testBasicChordSaveLoad(const TestCase& test) {
    setupTestEnvironment();
    
    // Create test chord state
    uint32_t testModifierMask = 0x05;
    std::vector<std::pair<uint32_t, std::string>> testChords = {
        {0x03, encodeTestMacro("\"hello\"")},
        {0x0C, encodeTestMacro("CTRL C")},
        {0x21, encodeTestMacro("\"world\"")}
    };
    
    setTestChordState(testModifierMask, testChords);
    
    // Save chords to EEPROM
    uint16_t startOffset = 50;
    uint16_t endOffset = saveChords(startOffset, testModifierMask, mockForEachChord);
    
    ASSERT_TRUE(endOffset > startOffset, "saveChords should return offset after saved data");
    
    // Clear mock system and load back
    mockChordSystem.reset();
    uint32_t loadedModifierMask = loadChords(startOffset, mockAddChord, mockClearAllChords);
    
    // FIXED: Pass the actual returned modifier mask to verification
    std::string verifyDebug;
    bool stateMatch = verifyChordState(loadedModifierMask, testModifierMask, testChords, verifyDebug);
    ASSERT_TRUE(stateMatch, "Loaded chord state should match saved state. " + verifyDebug);
}

void testSingleChordSaveLoad(const TestCase& test) {
    setupTestEnvironment();
    
    uint32_t testModifierMask = 0x01;
    std::vector<std::pair<uint32_t, std::string>> testChords = {
        {0x06, encodeTestMacro("+SHIFT \"CAPS\" -SHIFT")}
    };
    
    setTestChordState(testModifierMask, testChords);
    
    uint16_t startOffset = 0;
    uint16_t endOffset = saveChords(startOffset, testModifierMask, mockForEachChord);
    
    ASSERT_TRUE(endOffset > startOffset, "Single chord save should succeed");
    
    mockChordSystem.reset();
    uint32_t loadedModifierMask = loadChords(startOffset, mockAddChord, mockClearAllChords);
    
    // FIXED: Use actual returned modifier mask
    std::string verifyDebug;
    bool stateMatch = verifyChordState(loadedModifierMask, testModifierMask, testChords, verifyDebug);
    ASSERT_TRUE(stateMatch, "Single chord should be loaded correctly. " + verifyDebug);
}

void testEmptyChordListSaveLoad(const TestCase& test) {
    setupTestEnvironment();
    
    // Test with modifier mask but no chords
    uint32_t testModifierMask = 0x0A;
    std::vector<std::pair<uint32_t, std::string>> testChords; // Empty
    
    setTestChordState(testModifierMask, testChords);
    
    uint16_t startOffset = 200;
    uint16_t endOffset = saveChords(startOffset, testModifierMask, mockForEachChord);
    
    ASSERT_TRUE(endOffset > startOffset, "Empty chord list save should succeed");
    
    mockChordSystem.reset();
    uint32_t loadedModifierMask = loadChords(startOffset, mockAddChord, mockClearAllChords);
    
    ASSERT_EQ(loadedModifierMask, testModifierMask, "Empty chord list modifier mask should match");
    ASSERT_EQ(mockChordSystem.getChordCount(), 0, "No chords should be loaded");
}

//==============================================================================
// ADVANCED SAVE/LOAD TESTS
//==============================================================================

void testManyChordsSaveLoad(const TestCase& test) {
    setupTestEnvironment();
    
    uint32_t testModifierMask = 0x0F;
    std::vector<std::pair<uint32_t, std::string>> testChords;
    
    // Add many different chords
    for (int i = 0; i < 10; i++) {
        uint32_t keyMask = (1 << i) | (1 << (i + 10));
        std::string macro = encodeTestMacro("\"chord" + std::to_string(i) + "\"");
        testChords.push_back({keyMask, macro});
    }
    
    setTestChordState(testModifierMask, testChords);
    
    uint16_t startOffset = 300;
    uint16_t endOffset = saveChords(startOffset, testModifierMask, mockForEachChord);
    
    ASSERT_TRUE(endOffset > startOffset, "Many chords save should succeed");
    
    mockChordSystem.reset();
    uint32_t loadedModifierMask = loadChords(startOffset, mockAddChord, mockClearAllChords);
    
    // FIXED: Use actual returned modifier mask
    std::string verifyDebug;
    bool stateMatch = verifyChordState(loadedModifierMask, testModifierMask, testChords, verifyDebug);
    ASSERT_TRUE(stateMatch, "All chords should be loaded correctly. " + verifyDebug);
}

void testLongMacroSaveLoad(const TestCase& test) {
    setupTestEnvironment();
    
    // Test with a very long macro sequence
    std::string longMacroCommand = "CTRL A \"This is a very long macro sequence with lots of text to test the storage system's ability to handle longer strings without truncation or corruption. It includes special characters: !@#$%^&*()\" ENTER";
    std::string encodedLongMacro = encodeTestMacro(longMacroCommand);
    
    uint32_t testModifierMask = 0x02;
    std::vector<std::pair<uint32_t, std::string>> testChords = {
        {0x18, encodedLongMacro}
    };
    
    setTestChordState(testModifierMask, testChords);
    
    uint16_t startOffset = 400;
    uint16_t endOffset = saveChords(startOffset, testModifierMask, mockForEachChord);
    
    ASSERT_TRUE(endOffset > startOffset, "Long macro save should succeed");
    
    mockChordSystem.reset();
    uint32_t loadedModifierMask = loadChords(startOffset, mockAddChord, mockClearAllChords);
    
    // FIXED: Use actual returned modifier mask
    std::string verifyDebug;
    bool stateMatch = verifyChordState(loadedModifierMask, testModifierMask, testChords, verifyDebug);
    ASSERT_TRUE(stateMatch, "Long macro should be loaded correctly. " + verifyDebug);
}

void testSpecialCharacterMacros(const TestCase& test) {
    setupTestEnvironment();
    
    uint32_t testModifierMask = 0x04;
    std::vector<std::pair<uint32_t, std::string>> testChords = {
        {0x11, encodeTestMacro("\"line1\\nline2\\ttabbed\"")},
        {0x22, encodeTestMacro("\"quotes\\\"inside\\\"string\"")},
        {0x44, encodeTestMacro("\"backslash\\\\test\"")},
        {0x88, encodeTestMacro("ESC \"escape test\"")}
    };
    
    setTestChordState(testModifierMask, testChords);
    
    uint16_t startOffset = 500;
    uint16_t endOffset = saveChords(startOffset, testModifierMask, mockForEachChord);
    
    ASSERT_TRUE(endOffset > startOffset, "Special character macros save should succeed");
    
    mockChordSystem.reset();
    uint32_t loadedModifierMask = loadChords(startOffset, mockAddChord, mockClearAllChords);
    
    // FIXED: Use actual returned modifier mask
    std::string verifyDebug;
    bool stateMatch = verifyChordState(loadedModifierMask, testModifierMask, testChords, verifyDebug);
    ASSERT_TRUE(stateMatch, "Special character macros should be loaded correctly. " + verifyDebug);
}

//==============================================================================
// ERROR HANDLING AND EDGE CASE TESTS
//==============================================================================

void testInvalidMagicNumber(const TestCase& test) {
    setupTestEnvironment();
    
    // Write invalid magic number to EEPROM
    uint16_t startOffset = 100;
    EEPROM.put(startOffset, (uint32_t)0x12345678);
    
    // Try to load
    uint32_t modifierMask = loadChords(startOffset, mockAddChord, mockClearAllChords);
    
    ASSERT_EQ(modifierMask, 0, "Invalid magic number should return 0");
    ASSERT_EQ(mockChordSystem.getAddChordCallCount(), 0, "No chords should be added with invalid magic");
    
    std::string operationLog = mockChordSystem.getOperationLog();
    ASSERT_STR_CONTAINS(operationLog, "CLEAR_ALL", "clearAllChords should still be called");
}

void testCorruptedChordCount(const TestCase& test) {
    setupTestEnvironment();
    
    uint16_t startOffset = 150;
    uint16_t offset = startOffset;
    
    // Write valid magic number
    EEPROM.put(offset, CHORD_MAGIC_VALUE);
    offset += sizeof(uint32_t);
    
    // Write modifier mask
    EEPROM.put(offset, (uint32_t)0x01);
    offset += sizeof(uint32_t);
    
    // Write unreasonably large chord count
    EEPROM.put(offset, (uint32_t)9999);
    offset += sizeof(uint32_t);
    
    // Try to load
    uint32_t modifierMask = loadChords(startOffset, mockAddChord, mockClearAllChords);
    
    ASSERT_EQ(modifierMask, 0, "Corrupted chord count should return 0");
}

void testZeroModifierMask(const TestCase& test) {
    setupTestEnvironment();
    
    uint32_t testModifierMask = 0;
    std::vector<std::pair<uint32_t, std::string>> testChords = {
        {0x03, encodeTestMacro("\"no modifiers\"")}
    };
    
    setTestChordState(testModifierMask, testChords);
    
    uint16_t startOffset = 600;
    uint16_t endOffset = saveChords(startOffset, testModifierMask, mockForEachChord);
    
    ASSERT_TRUE(endOffset > startOffset, "Zero modifier mask save should succeed");
    
    mockChordSystem.reset();
    uint32_t loadedModifierMask = loadChords(startOffset, mockAddChord, mockClearAllChords);
    
    ASSERT_EQ(loadedModifierMask, 0, "Zero modifier mask should be loaded correctly");
    
    // FIXED: Use actual returned modifier mask
    std::string verifyDebug;
    bool stateMatch = verifyChordState(loadedModifierMask, testModifierMask, testChords, verifyDebug);
    ASSERT_TRUE(stateMatch, "Chord with zero modifier mask should load correctly. " + verifyDebug);
}

void testMaxModifierMask(const TestCase& test) {
    setupTestEnvironment();
    
    uint32_t testModifierMask = 0xFFFFFFFF;
    std::vector<std::pair<uint32_t, std::string>> testChords = {
        {0x01, encodeTestMacro("\"max modifiers\"")}
    };
    
    setTestChordState(testModifierMask, testChords);
    
    uint16_t startOffset = 700;
    uint16_t endOffset = saveChords(startOffset, testModifierMask, mockForEachChord);
    
    ASSERT_TRUE(endOffset > startOffset, "Max modifier mask save should succeed");
    
    mockChordSystem.reset();
    uint32_t loadedModifierMask = loadChords(startOffset, mockAddChord, mockClearAllChords);
    
    // FIXED: Use actual returned modifier mask
    std::string verifyDebug;
    bool stateMatch = verifyChordState(loadedModifierMask, testModifierMask, testChords, verifyDebug);
    ASSERT_TRUE(stateMatch, "Chord with max modifier mask should load correctly. " + verifyDebug);
}

//==============================================================================
// INTEGRATION TESTS
//==============================================================================

void testMultipleSaveLoadCycles(const TestCase& test) {
    setupTestEnvironment();
    
    uint16_t startOffset = 800;
    
    for (int cycle = 0; cycle < 3; cycle++) {
        uint32_t testModifierMask = cycle + 1;
        std::vector<std::pair<uint32_t, std::string>> testChords;
        
        for (int i = 0; i < cycle + 2; i++) {
            uint32_t keyMask = (1 << i) | (1 << (i + 5));
            std::string macro = encodeTestMacro("\"cycle" + std::to_string(cycle) + "_chord" + std::to_string(i) + "\"");
            testChords.push_back({keyMask, macro});
        }
        
        setTestChordState(testModifierMask, testChords);
        
        // Save
        uint16_t endOffset = saveChords(startOffset, testModifierMask, mockForEachChord);
        ASSERT_TRUE(endOffset > startOffset, "Save cycle " + std::to_string(cycle) + " should succeed");
        
        // Load back
        mockChordSystem.reset();
        uint32_t loadedModifierMask = loadChords(startOffset, mockAddChord, mockClearAllChords);
        
        // FIXED: Use actual returned modifier mask
        std::string verifyDebug;
        bool stateMatch = verifyChordState(loadedModifierMask, testModifierMask, testChords, verifyDebug);
        ASSERT_TRUE(stateMatch, "Cycle " + std::to_string(cycle) + " chord state should match. " + verifyDebug);
    }
}

void testStorageOffsetChaining(const TestCase& test) {
    setupTestEnvironment();
    
    // First save
    uint32_t testModifierMask1 = 0x01;
    std::vector<std::pair<uint32_t, std::string>> testChords1 = {
        {0x03, encodeTestMacro("\"first\"")}
    };
    setTestChordState(testModifierMask1, testChords1);
    
    uint16_t offset1 = saveChords(50, testModifierMask1, mockForEachChord);
    ASSERT_TRUE(offset1 > 50, "First save should advance offset");
    
    // Second save (chained) - FIXED: Reset mock state properly for independent saves
    mockChordSystem.reset();
    uint32_t testModifierMask2 = 0x02;
    std::vector<std::pair<uint32_t, std::string>> testChords2 = {
        {0x0C, encodeTestMacro("\"second\"")}
    };
    setTestChordState(testModifierMask2, testChords2);
    
    uint16_t offset2 = saveChords(offset1, testModifierMask2, mockForEachChord);
    ASSERT_TRUE(offset2 > offset1, "Second save should advance offset further");
    
    // Load both back independently
    mockChordSystem.reset();
    uint32_t loaded1 = loadChords(50, mockAddChord, mockClearAllChords);
    ASSERT_EQ(loaded1, testModifierMask1, "First saved data should load correctly");
    ASSERT_EQ(mockChordSystem.getChordCount(), 1, "First save should have one chord");
    ASSERT_TRUE(mockChordSystem.hasChord(0x03), "First chord should be present");
    
    mockChordSystem.reset();
    uint32_t loaded2 = loadChords(offset1, mockAddChord, mockClearAllChords);
    ASSERT_EQ(loaded2, testModifierMask2, "Second saved data should load correctly");
    ASSERT_EQ(mockChordSystem.getChordCount(), 1, "Second save should have one chord");
    ASSERT_TRUE(mockChordSystem.hasChord(0x0C), "Second chord should be present");
}

//==============================================================================
// TEST CASE DEFINITIONS AND RUNNER
//==============================================================================

std::vector<std::pair<TestCase, void(*)(const TestCase&)>> createAllChordStorageTests() {
    return {
        // Basic functionality tests
        {TestCase("Empty chord storage load", "", EXPECT_PASS), testEmptyChordStorage},
        {TestCase("Basic chord save/load", "", EXPECT_PASS), testBasicChordSaveLoad},
        {TestCase("Single chord save/load", "", EXPECT_PASS), testSingleChordSaveLoad},
        {TestCase("Empty chord list save/load", "", EXPECT_PASS), testEmptyChordListSaveLoad},
        
        // Advanced functionality tests
        {TestCase("Many chords save/load", "", EXPECT_PASS), testManyChordsSaveLoad},
        {TestCase("Long macro save/load", "", EXPECT_PASS), testLongMacroSaveLoad},
        {TestCase("Special character macros", "", EXPECT_PASS), testSpecialCharacterMacros},
        
        // Error handling tests
        {TestCase("Invalid magic number", "", EXPECT_PASS), testInvalidMagicNumber},
        {TestCase("Corrupted chord count", "", EXPECT_PASS), testCorruptedChordCount},
        {TestCase("Zero modifier mask", "", EXPECT_PASS), testZeroModifierMask},
        {TestCase("Max modifier mask", "", EXPECT_PASS), testMaxModifierMask},
        
        // Integration tests
        {TestCase("Multiple save/load cycles", "", EXPECT_PASS), testMultipleSaveLoadCycles},
        {TestCase("Storage offset chaining", "", EXPECT_PASS), testStorageOffsetChaining},
    };
}

//==============================================================================
// MAIN TEST RUNNER
//==============================================================================

int main(int argc, char* argv[]) {
    bool verbose = (argc > 1 && strcmp(argv[1], "-v") == 0);
    
    std::cout << "Running Fixed Chord Storage System Tests" << std::endl;
    std::cout << "========================================" << std::endl << std::endl;
    
    TestRunner runner(verbose);
    
    auto allTests = createAllChordStorageTests();
    
    for (const auto& testPair : allTests) {
        runner.runTest(testPair.first, testPair.second);
    }
    
    std::cout << std::endl;
    runner.printSummary();
    
    if (verbose) {
        std::cout << std::endl << "Test completed with all fixes applied" << std::endl;
    }
    
    return runner.allPassed() ? 0 : 1;
}