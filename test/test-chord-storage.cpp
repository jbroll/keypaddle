/*
 * Chord Storage System Testing
 * Tests EEPROM save/load functionality for chord patterns and modifier configuration
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
// TEST DATA STRUCTURES
//==============================================================================

struct TestChordPattern {
    uint32_t keyMask;
    std::string macro;
    
    TestChordPattern(uint32_t mask, const std::string& macroStr) 
        : keyMask(mask), macro(macroStr) {}
};

struct TestChordState {
    uint32_t modifierMask;
    std::vector<TestChordPattern> chords;
    
    TestChordState(uint32_t modMask = 0) : modifierMask(modMask) {}
    
    void addChord(uint32_t keyMask, const std::string& macro) {
        chords.emplace_back(keyMask, macro);
    }
};

//==============================================================================
// MOCK CHORD SYSTEM IMPLEMENTATION FOR TESTING
//==============================================================================

class MockChordSystem {
private:
    std::map<uint32_t, std::string> chordMap;
    uint32_t currentModifierMask;
    bool addChordCallCount;
    bool clearAllChordsCallCount;
    
public:
    MockChordSystem() : currentModifierMask(0), addChordCallCount(0), clearAllChordsCallCount(0) {}
    
    void reset() {
        chordMap.clear();
        currentModifierMask = 0;
        addChordCallCount = 0;
        clearAllChordsCallCount = 0;
    }
    
    void setModifierMask(uint32_t mask) {
        currentModifierMask = mask;
    }
    
    uint32_t getModifierMask() const {
        return currentModifierMask;
    }
    
    bool addChord(uint32_t keyMask, const char* macroSequence) {
        addChordCallCount++;
        if (!macroSequence) return false;
        chordMap[keyMask] = std::string(macroSequence);
        return true;
    }
    
    void clearAllChords() {
        clearAllChordsCallCount++;
        chordMap.clear();
    }
    
    void forEachChord(void (*callback)(uint32_t keyMask, const char* macro)) {
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
};

// Global mock instance
MockChordSystem mockChordSystem;

//==============================================================================
// CALLBACK FUNCTION IMPLEMENTATIONS FOR TESTING
//==============================================================================

// Callback for saveChords - iterates through mock chord system
void mockForEachChord(void (*callback)(uint32_t keyMask, const char* macro)) {
    mockChordSystem.forEachChord(callback);
}

// Callback for loadChords - adds chord to mock system
bool mockAddChord(uint32_t keyMask, const char* macroSequence) {
    return mockChordSystem.addChord(keyMask, macroSequence);
}

// Callback for loadChords - clears mock system
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

void setTestChordState(const TestChordState& state) {
    mockChordSystem.setModifierMask(state.modifierMask);
    for (const auto& chord : state.chords) {
        mockChordSystem.addChord(chord.keyMask, chord.macro.c_str());
    }
}

bool verifyChordState(const TestChordState& expected) {
    // Check modifier mask
    if (mockChordSystem.getModifierMask() != expected.modifierMask) {
        return false;
    }
    
    // Check chord count
    if (mockChordSystem.getChordCount() != expected.chords.size()) {
        return false;
    }
    
    // Check each chord
    for (const auto& expectedChord : expected.chords) {
        if (!mockChordSystem.hasChord(expectedChord.keyMask)) {
            return false;
        }
        
        std::string actualMacro = mockChordSystem.getChordMacro(expectedChord.keyMask);
        if (actualMacro != expectedChord.macro) {
            return false;
        }
    }
    
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
// BASIC SAVE/LOAD TESTS
//==============================================================================

void testEmptyChordStorage(const TestCase& test) {
    setupTestEnvironment();
    
    // Try to load from empty EEPROM
    uint16_t startOffset = 100; // Use arbitrary offset
    uint32_t modifierMask = loadChords(startOffset, mockAddChord, mockClearAllChords);
    
    // Should return 0 (no valid chord data)
    ASSERT_EQ(modifierMask, 0, "loadChords should return 0 for empty EEPROM");
    
    // clearAllChords should have been called
    ASSERT_TRUE(mockChordSystem.getClearAllChordsCallCount() > 0, "clearAllChords should be called during load");
    
    // No chords should be added
    ASSERT_EQ(mockChordSystem.getAddChordCallCount(), 0, "No chords should be added from empty EEPROM");
}

void testBasicChordSaveLoad(const TestCase& test) {
    setupTestEnvironment();
    
    // Create test chord state
    TestChordState testState(0x05); // Modifier mask with bits 0 and 2 set
    testState.addChord(0x03, encodeTestMacro("\"hello\""));        // Keys 0+1
    testState.addChord(0x0C, encodeTestMacro("CTRL C"));           // Keys 2+3
    testState.addChord(0x21, encodeTestMacro("\"world\""));        // Keys 0+5
    
    setTestChordState(testState);
    
    // Save chords to EEPROM
    uint16_t startOffset = 50;
    uint16_t endOffset = saveChords(startOffset, testState.modifierMask, mockForEachChord);
    
    ASSERT_TRUE(endOffset > startOffset, "saveChords should return offset after saved data");
    
    // Clear mock system and load back
    mockChordSystem.reset();
    uint32_t loadedModifierMask = loadChords(startOffset, mockAddChord, mockClearAllChords);
    
    // Verify loaded data
    ASSERT_EQ(loadedModifierMask, testState.modifierMask, "Loaded modifier mask should match saved");
    ASSERT_TRUE(verifyChordState(testState), "Loaded chord state should match saved state");
}

void testSingleChordSaveLoad(const TestCase& test) {
    setupTestEnvironment();
    
    // Test with just one chord
    TestChordState testState(0x01); // Just modifier bit 0
    testState.addChord(0x06, encodeTestMacro("+SHIFT \"CAPS\" -SHIFT")); // Keys 1+2
    
    setTestChordState(testState);
    
    uint16_t startOffset = 0;
    uint16_t endOffset = saveChords(startOffset, testState.modifierMask, mockForEachChord);
    
    ASSERT_TRUE(endOffset > startOffset, "Single chord save should succeed");
    
    mockChordSystem.reset();
    uint32_t loadedModifierMask = loadChords(startOffset, mockAddChord, mockClearAllChords);
    
    ASSERT_EQ(loadedModifierMask, testState.modifierMask, "Single chord modifier mask should match");
    ASSERT_TRUE(verifyChordState(testState), "Single chord should be loaded correctly");
}

void testEmptyChordListSaveLoad(const TestCase& test) {
    setupTestEnvironment();
    
    // Test with modifier mask but no chords
    uint32_t modifierMask = 0x0A; // Bits 1 and 3
    TestChordState testState(modifierMask);
    // No chords added
    
    setTestChordState(testState);
    
    uint16_t startOffset = 200;
    uint16_t endOffset = saveChords(startOffset, modifierMask, mockForEachChord);
    
    ASSERT_TRUE(endOffset > startOffset, "Empty chord list save should succeed");
    
    mockChordSystem.reset();
    uint32_t loadedModifierMask = loadChords(startOffset, mockAddChord, mockClearAllChords);
    
    ASSERT_EQ(loadedModifierMask, modifierMask, "Empty chord list modifier mask should match");
    ASSERT_EQ(mockChordSystem.getChordCount(), 0, "No chords should be loaded");
}

//==============================================================================
// ADVANCED SAVE/LOAD TESTS
//==============================================================================

void testManyChordsSaveLoad(const TestCase& test) {
    setupTestEnvironment();
    
    // Test with many chords
    TestChordState testState(0x0F); // All modifier bits 0-3
    
    // Add many different chords
    for (int i = 0; i < 10; i++) {
        uint32_t keyMask = (1 << i) | (1 << (i + 10)); // Two keys each
        std::string macro = "\"chord" + std::to_string(i) + "\"";
        testState.addChord(keyMask, encodeTestMacro(macro));
    }
    
    setTestChordState(testState);
    
    uint16_t startOffset = 300;
    uint16_t endOffset = saveChords(startOffset, testState.modifierMask, mockForEachChord);
    
    ASSERT_TRUE(endOffset > startOffset, "Many chords save should succeed");
    
    mockChordSystem.reset();
    uint32_t loadedModifierMask = loadChords(startOffset, mockAddChord, mockClearAllChords);
    
    ASSERT_EQ(loadedModifierMask, testState.modifierMask, "Many chords modifier mask should match");
    ASSERT_TRUE(verifyChordState(testState), "All chords should be loaded correctly");
}

void testLongMacroSaveLoad(const TestCase& test) {
    setupTestEnvironment();
    
    // Test with a very long macro sequence
    std::string longMacroCommand = "CTRL A \"This is a very long macro sequence with lots of text to test the storage system's ability to handle longer strings without truncation or corruption. It includes special characters: !@#$%^&*()\" ENTER";
    std::string encodedLongMacro = encodeTestMacro(longMacroCommand);
    
    TestChordState testState(0x02);
    testState.addChord(0x18, encodedLongMacro); // Keys 3+4
    
    setTestChordState(testState);
    
    uint16_t startOffset = 400;
    uint16_t endOffset = saveChords(startOffset, testState.modifierMask, mockForEachChord);
    
    ASSERT_TRUE(endOffset > startOffset, "Long macro save should succeed");
    
    mockChordSystem.reset();
    uint32_t loadedModifierMask = loadChords(startOffset, mockAddChord, mockClearAllChords);
    
    ASSERT_EQ(loadedModifierMask, testState.modifierMask, "Long macro modifier mask should match");
    ASSERT_TRUE(verifyChordState(testState), "Long macro should be loaded correctly");
}

void testSpecialCharacterMacros(const TestCase& test) {
    setupTestEnvironment();
    
    // Test with macros containing special characters and escape sequences
    TestChordState testState(0x04);
    testState.addChord(0x11, encodeTestMacro("\"line1\\nline2\\ttabbed\""));
    testState.addChord(0x22, encodeTestMacro("\"quotes\\\"inside\\\"string\""));
    testState.addChord(0x44, encodeTestMacro("\"backslash\\\\test\""));
    testState.addChord(0x88, encodeTestMacro("ESC \"escape test\""));
    
    setTestChordState(testState);
    
    uint16_t startOffset = 500;
    uint16_t endOffset = saveChords(startOffset, testState.modifierMask, mockForEachChord);
    
    ASSERT_TRUE(endOffset > startOffset, "Special character macros save should succeed");
    
    mockChordSystem.reset();
    uint32_t loadedModifierMask = loadChords(startOffset, mockAddChord, mockClearAllChords);
    
    ASSERT_EQ(loadedModifierMask, testState.modifierMask, "Special character modifier mask should match");
    ASSERT_TRUE(verifyChordState(testState), "Special character macros should be loaded correctly");
}

//==============================================================================
// ERROR HANDLING AND EDGE CASE TESTS
//==============================================================================

void testInvalidMagicNumber(const TestCase& test) {
    setupTestEnvironment();
    
    // Write invalid magic number to EEPROM
    uint16_t startOffset = 100;
    EEPROM.put(startOffset, (uint32_t)0x12345678); // Wrong magic
    
    // Try to load
    uint32_t modifierMask = loadChords(startOffset, mockAddChord, mockClearAllChords);
    
    // Should return 0 (no valid data)
    ASSERT_EQ(modifierMask, 0, "Invalid magic number should return 0");
    ASSERT_EQ(mockChordSystem.getAddChordCallCount(), 0, "No chords should be added with invalid magic");
}

void testCorruptedChordCount(const TestCase& test) {
    setupTestEnvironment();
    
    uint16_t startOffset = 150;
    uint16_t offset = startOffset;
    
    // Write valid magic number
    offset = startOffset;
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
    
    // Should return 0 due to sanity check failure
    ASSERT_EQ(modifierMask, 0, "Corrupted chord count should return 0");
}

void testZeroModifierMask(const TestCase& test) {
    setupTestEnvironment();
    
    // Test with zero modifier mask (valid case)
    TestChordState testState(0); // No modifier keys
    testState.addChord(0x03, encodeTestMacro("\"no modifiers\""));
    
    setTestChordState(testState);
    
    uint16_t startOffset = 600;
    uint16_t endOffset = saveChords(startOffset, testState.modifierMask, mockForEachChord);
    
    ASSERT_TRUE(endOffset > startOffset, "Zero modifier mask save should succeed");
    
    mockChordSystem.reset();
    uint32_t loadedModifierMask = loadChords(startOffset, mockAddChord, mockClearAllChords);
    
    ASSERT_EQ(loadedModifierMask, 0, "Zero modifier mask should be loaded correctly");
    ASSERT_TRUE(verifyChordState(testState), "Chord with zero modifier mask should load correctly");
}

void testMaxModifierMask(const TestCase& test) {
    setupTestEnvironment();
    
    // Test with maximum modifier mask
    TestChordState testState(0xFFFFFFFF); // All bits set
    testState.addChord(0x01, encodeTestMacro("\"max modifiers\""));
    
    setTestChordState(testState);
    
    uint16_t startOffset = 700;
    uint16_t endOffset = saveChords(startOffset, testState.modifierMask, mockForEachChord);
    
    ASSERT_TRUE(endOffset > startOffset, "Max modifier mask save should succeed");
    
    mockChordSystem.reset();
    uint32_t loadedModifierMask = loadChords(startOffset, mockAddChord, mockClearAllChords);
    
    ASSERT_EQ(loadedModifierMask, testState.modifierMask, "Max modifier mask should be loaded correctly");
    ASSERT_TRUE(verifyChordState(testState), "Chord with max modifier mask should load correctly");
}

//==============================================================================
// INTEGRATION TESTS
//==============================================================================

void testMultipleSaveLoadCycles(const TestCase& test) {
    setupTestEnvironment();
    
    uint16_t startOffset = 800;
    
    for (int cycle = 0; cycle < 3; cycle++) {
        // Create different test state for each cycle
        TestChordState testState(cycle + 1);
        
        for (int i = 0; i < cycle + 2; i++) {
            uint32_t keyMask = (1 << i) | (1 << (i + 5));
            std::string macro = "\"cycle" + std::to_string(cycle) + "_chord" + std::to_string(i) + "\"";
            testState.addChord(keyMask, encodeTestMacro(macro));
        }
        
        setTestChordState(testState);
        
        // Save
        uint16_t endOffset = saveChords(startOffset, testState.modifierMask, mockForEachChord);
        ASSERT_TRUE(endOffset > startOffset, "Save cycle " + std::to_string(cycle) + " should succeed");
        
        // Load back
        mockChordSystem.reset();
        uint32_t loadedModifierMask = loadChords(startOffset, mockAddChord, mockClearAllChords);
        
        // Verify
        ASSERT_EQ(loadedModifierMask, testState.modifierMask, "Cycle " + std::to_string(cycle) + " modifier mask should match");
        ASSERT_TRUE(verifyChordState(testState), "Cycle " + std::to_string(cycle) + " chord state should match");
    }
}

void testStorageOffsetChaining(const TestCase& test) {
    setupTestEnvironment();
    
    // Test that multiple chord saves can be chained using returned offsets
    uint16_t currentOffset = 50;
    
    // First save
    TestChordState state1(0x01);
    state1.addChord(0x03, encodeTestMacro("\"first\""));
    setTestChordState(state1);
    
    uint16_t offset1 = saveChords(currentOffset, state1.modifierMask, mockForEachChord);
    ASSERT_TRUE(offset1 > currentOffset, "First save should advance offset");
    
    // Second save (chained)
    TestChordState state2(0x02);
    state2.addChord(0x0C, encodeTestMacro("\"second\""));
    setTestChordState(state2);
    
    uint16_t offset2 = saveChords(offset1, state2.modifierMask, mockForEachChord);
    ASSERT_TRUE(offset2 > offset1, "Second save should advance offset further");
    
    // Load both back independently
    mockChordSystem.reset();
    uint32_t loaded1 = loadChords(currentOffset, mockAddChord, mockClearAllChords);
    ASSERT_EQ(loaded1, state1.modifierMask, "First saved data should load correctly");
    ASSERT_EQ(mockChordSystem.getChordCount(), 1, "First save should have one chord");
    ASSERT_TRUE(mockChordSystem.hasChord(0x03), "First chord should be present");
    
    mockChordSystem.reset();
    uint32_t loaded2 = loadChords(offset1, mockAddChord, mockClearAllChords);
    ASSERT_EQ(loaded2, state2.modifierMask, "Second saved data should load correctly");
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
    
    std::cout << "Running Chord Storage System Tests" << std::endl;
    std::cout << "===================================" << std::endl << std::endl;
    
    TestRunner runner(verbose);
    
    auto allTests = createAllChordStorageTests();
    
    for (const auto& testPair : allTests) {
        runner.runTest(testPair.first, testPair.second);
    }
    
    std::cout << std::endl;
    runner.printSummary();
    
    if (verbose) {
        std::cout << std::endl << "Chord Storage Details:" << std::endl;
        std::cout << "Magic value: 0x" << std::hex << CHORD_MAGIC_VALUE << std::dec << std::endl;
        std::cout << "EEPROM usage: " << EEPROM.countUsedBytes() << "/" << EEPROM.length() << " bytes" << std::endl;
        std::cout << "Test completed with mock chord system" << std::endl;
    }
    
    return runner.allPassed() ? 0 : 1;
}