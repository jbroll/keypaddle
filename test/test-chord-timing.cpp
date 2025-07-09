/*
 * Chord Timing and Release Behavior Testing - FINAL FIX
 * 
 * Tests that chord patterns only execute when ALL non-modifier keys are released,
 * not on partial releases during the chord sequence.
 * 
 * FIXED: Proper state reset between tests
 */

#include "Arduino.h"
#include "EEPROM.h"
#include "micro-test.h"

// Include the actual implementation files from parent directory
#include "../config.h"
#include "../storage.h"
#include "../chordStorage.h"
#include "../chording.h"
#include "../macro-encode.h"

#include <iostream>
#include <vector>
#include <string>

void setupTestEnvironment() {
    // Clear all state
    EEPROM.clear();
    Keyboard.clearActions();
    
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
    
    // Initialize systems
    setupStorage();
    setupChording();
    
    // Clear the chording engine state
    chording.clearAllChords();
    chording.clearAllModifiers();
    
    // CRITICAL FIX: Force reset the chording state by simulating all keys released
    // This ensures we start each test in CHORD_IDLE state
    processChording(0x00);  // All keys released - forces reset to IDLE state
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

void addTestChord(uint32_t keyMask, const std::string& macroCommand) {
    std::string encoded = encodeTestMacro(macroCommand);
    if (!encoded.empty()) {
        chording.addChord(keyMask, encoded.c_str());
    }
}

// Helper function to check if keyboard output contains expected characters
bool keyboardOutputContains(const std::string& output, const std::string& expectedText) {
    // The mock keyboard outputs like "write t write e write s write t"
    // We need to check if the individual characters are present in sequence
    
    for (char c : expectedText) {
        std::string searchFor = std::string("write ") + c;
        if (output.find(searchFor) == std::string::npos) {
            return false;
        }
    }
    return true;
}

// Helper function to count character sequences in keyboard output
int countCharacterSequence(const std::string& output, const std::string& expectedText) {
    // Simple approach: count occurrences of the first character and verify the sequence follows
    if (expectedText.empty()) return 0;
    
    char firstChar = expectedText[0];
    std::string searchFor = std::string("write ") + firstChar;
    
    size_t pos = 0;
    int count = 0;
    
    while ((pos = output.find(searchFor, pos)) != std::string::npos) {
        // Found the first character, check if the full sequence follows
        size_t checkPos = pos;
        bool fullSequenceFound = true;
        
        for (char c : expectedText) {
            std::string charPattern = std::string("write ") + c;
            checkPos = output.find(charPattern, checkPos);
            if (checkPos == std::string::npos) {
                fullSequenceFound = false;
                break;
            }
            checkPos += charPattern.length();
        }
        
        if (fullSequenceFound) {
            count++;
        }
        
        pos += searchFor.length();
    }
    
    return count;
}

// Simulate a sequence of switch state changes
struct SwitchEvent {
    uint32_t switchState;
    std::string description;
    
    SwitchEvent(uint32_t state, const std::string& desc) 
        : switchState(state), description(desc) {}
};

std::vector<std::string> simulateSwitchSequence(const std::vector<SwitchEvent>& events) {
    std::vector<std::string> keyboardOutputs;
    
    for (const auto& event : events) {
        // Clear keyboard before each event
        Keyboard.clearActions();
        
        // Process the switch state change
        bool chordHandled = processChording(event.switchState);
        
        // Capture keyboard output if any
        std::string output = Keyboard.toString();
        if (!output.empty()) {
            keyboardOutputs.push_back(output);
        }
    }
    
    return keyboardOutputs;
}

//==============================================================================
// CHORD TIMING TESTS
//==============================================================================

void testBasicChordTiming(const TestCase& test) {
    setupTestEnvironment();
    
    // Add a chord: keys 6+7 → "test"
    addTestChord(0xC0, "\"test\"");  // 0xC0 = keys 6+7
    
    // Simulate the exact sequence from the user's log
    std::vector<SwitchEvent> sequence = {
        {0x80, "Key 7 pressed"},
        {0xC0, "Keys 6+7 pressed (chord formed)"},
        {0x80, "Key 6 released (should NOT trigger)"},
        {0x00, "Key 7 released (should trigger chord)"}
    };
    
    auto outputs = simulateSwitchSequence(sequence);
    
    // Should only get ONE output when the last key is released
    ASSERT_EQ(outputs.size(), 1, "Should have exactly one chord execution");
    ASSERT_TRUE(keyboardOutputContains(outputs[0], "test"), "Should contain chord output 'test'");
}

void testMultiplePartialReleases(const TestCase& test) {
    setupTestEnvironment();
    
    // Add a chord: keys 0+1+2 → "triple"
    addTestChord(0x07, "\"triple\"");  // 0x07 = keys 0+1+2
    
    // Test sequence with multiple partial releases
    std::vector<SwitchEvent> sequence = {
        {0x01, "Key 0 pressed"},
        {0x03, "Keys 0+1 pressed"},
        {0x07, "Keys 0+1+2 pressed (full chord)"},
        {0x06, "Key 0 released (partial)"},
        {0x04, "Key 1 released (partial)"},
        {0x00, "Key 2 released (complete - should trigger)"}
    };
    
    auto outputs = simulateSwitchSequence(sequence);
    
    // Should only get output on the final release
    ASSERT_EQ(outputs.size(), 1, "Should have exactly one execution despite multiple releases");
    ASSERT_TRUE(keyboardOutputContains(outputs[0], "triple"), "Should contain correct chord output");
}

void testOverlappingChords(const TestCase& test) {
    setupTestEnvironment();
    
    // Add multiple chords with overlapping keys
    addTestChord(0x03, "\"two\"");    // Keys 0+1
    addTestChord(0x07, "\"three\"");  // Keys 0+1+2
    
    // Test that the maximum pattern (3 keys) is captured and executed
    std::vector<SwitchEvent> sequence = {
        {0x01, "Key 0 pressed"},
        {0x03, "Keys 0+1 pressed (forms 2-key chord)"},
        {0x07, "Keys 0+1+2 pressed (forms 3-key chord - should override)"},
        {0x06, "Key 0 released"},
        {0x04, "Key 1 released"},
        {0x00, "Key 2 released (should trigger 3-key chord)"}
    };
    
    auto outputs = simulateSwitchSequence(sequence);
    
    // Should execute the 3-key chord, not the 2-key chord
    ASSERT_EQ(outputs.size(), 1, "Should have exactly one execution");
    ASSERT_TRUE(keyboardOutputContains(outputs[0], "three"), "Should execute the longer chord pattern");
}

void testModifierKeysIgnored(const TestCase& test) {
    setupTestEnvironment();
    
    // Set key 7 as a modifier
    chording.setModifierKey(7, true);
    
    // Add chord with modifier: keys 6+7 (where 7 is modifier)
    addTestChord(0xC0, "\"modified\"");  // Keys 6+7
    
    // Test sequence where modifier key is released but non-modifier remains
    std::vector<SwitchEvent> sequence = {
        {0x40, "Key 6 pressed (non-modifier)"},
        {0xC0, "Keys 6+7 pressed (6=non-mod, 7=modifier)"},
        {0x40, "Key 7 released (modifier released, should NOT trigger)"},
        {0x00, "Key 6 released (non-modifier released, should trigger)"}
    };
    
    auto outputs = simulateSwitchSequence(sequence);
    
    // Should only trigger when the non-modifier key is released
    ASSERT_EQ(outputs.size(), 1, "Should execute when non-modifier key released");
    ASSERT_TRUE(keyboardOutputContains(outputs[0], "modified"), "Should contain chord output");
}

void testNoChordMatch(const TestCase& test) {
    setupTestEnvironment();
    
    // Add a specific chord
    addTestChord(0x03, "\"match\"");  // Keys 0+1
    
    // Test sequence that forms a different pattern
    std::vector<SwitchEvent> sequence = {
        {0x04, "Key 2 pressed"},
        {0x0C, "Keys 2+3 pressed (no matching chord)"},
        {0x08, "Key 2 released"},
        {0x00, "Key 3 released (should not trigger any chord)"}
    };
    
    auto outputs = simulateSwitchSequence(sequence);
    
    // Should have no chord output
    ASSERT_EQ(outputs.size(), 0, "Should have no chord execution for unmatched pattern");
}

void testRapidPressRelease(const TestCase& test) {
    setupTestEnvironment();
    
    // Add a chord
    addTestChord(0x18, "\"rapid\"");  // Keys 3+4
    
    // Test rapid press and release sequence
    std::vector<SwitchEvent> sequence = {
        {0x08, "Key 3 pressed"},
        {0x18, "Keys 3+4 pressed"},
        {0x08, "Key 4 released immediately"},
        {0x00, "Key 3 released"}
    };
    
    auto outputs = simulateSwitchSequence(sequence);
    
    // Should still capture and execute the chord
    ASSERT_EQ(outputs.size(), 1, "Should execute chord even with rapid release");
    ASSERT_TRUE(keyboardOutputContains(outputs[0], "rapid"), "Should contain chord output");
}

void testUserReportedSequence(const TestCase& test) {
    setupTestEnvironment();
    
    // Reproduce the exact user scenario: keys 6+7 → "lll"
    addTestChord(0xC0, "\"lll\"");  // 0xC0 = keys 6+7 (bits 6 and 7)
    
    // Simulate the exact switch states from user's log
    std::vector<SwitchEvent> sequence = {
        {0x80, "Switches 0x80 (key 7)"},
        {0xC0, "Switches 0xC0 (keys 6+7)"},
        {0x80, "Switches 0x80 (key 6 released)"},
        {0x00, "Switches 0x0 (key 7 released)"}
    };
    
    auto outputs = simulateSwitchSequence(sequence);
    
    // Should produce exactly one "lll" output
    ASSERT_EQ(outputs.size(), 1, "Should execute chord exactly once");
    ASSERT_TRUE(keyboardOutputContains(outputs[0], "lll"), "Should contain 'lll' characters");
    
    // Count the number of 'l' characters in the output
    int lCount = countCharacterSequence(outputs[0], "l");
    ASSERT_EQ(lCount, 3, "Should output exactly 3 'l' characters");
}

void testComplexReleasePattern(const TestCase& test) {
    setupTestEnvironment();
    
    // Add a 4-key chord
    addTestChord(0x0F, "\"four\"");  // Keys 0+1+2+3
    
    // Test complex release pattern (release in different order than pressed)
    std::vector<SwitchEvent> sequence = {
        {0x01, "Key 0 pressed"},
        {0x03, "Key 1 pressed"},
        {0x07, "Key 2 pressed"},
        {0x0F, "Key 3 pressed (full 4-key chord)"},
        {0x0E, "Key 0 released first"},
        {0x0C, "Key 1 released second"},
        {0x08, "Key 2 released third"},
        {0x00, "Key 3 released last (should trigger)"}
    };
    
    auto outputs = simulateSwitchSequence(sequence);
    
    // Should execute the 4-key chord when the last key is released
    ASSERT_EQ(outputs.size(), 1, "Should execute once when last key released");
    ASSERT_TRUE(keyboardOutputContains(outputs[0], "four"), "Should execute 4-key chord");
}

//==============================================================================
// ERROR AND EDGE CASE TESTS
//==============================================================================

void testPartialChordNoExecution(const TestCase& test) {
    setupTestEnvironment();
    
    // Add a 3-key chord
    addTestChord(0x07, "\"complete\"");  // Keys 0+1+2
    
    // Press only 2 keys, then release (should not execute)
    std::vector<SwitchEvent> sequence = {
        {0x01, "Key 0 pressed"},
        {0x03, "Key 1 pressed (only 2 keys, not complete chord)"},
        {0x01, "Key 1 released"},
        {0x00, "Key 0 released"}
    };
    
    auto outputs = simulateSwitchSequence(sequence);
    
    // Should not execute anything (pattern doesn't match)
    ASSERT_EQ(outputs.size(), 0, "Should not execute partial chord");
}

void testInterruptedChord(const TestCase& test) {
    setupTestEnvironment();
    
    // Add a chord
    addTestChord(0x06, "\"interrupted\"");  // Keys 1+2
    
    // Start chord, then press additional key that breaks the pattern
    std::vector<SwitchEvent> sequence = {
        {0x02, "Key 1 pressed"},
        {0x06, "Key 2 pressed (forms valid chord)"},
        {0x0E, "Key 3 pressed (breaks chord pattern)"},
        {0x0C, "Key 1 released"},
        {0x08, "Key 2 released"},
        {0x00, "Key 3 released"}
    };
    
    auto outputs = simulateSwitchSequence(sequence);
    
    // Should not execute the original chord (pattern was exceeded)
    ASSERT_EQ(outputs.size(), 0, "Should not execute interrupted chord");
}

//==============================================================================
// TEST CASE DEFINITIONS AND RUNNER
//==============================================================================

std::vector<std::pair<TestCase, void(*)(const TestCase&)>> createChordTimingTests() {
    return {
        // Basic timing tests
        {TestCase("Basic chord timing", "", EXPECT_PASS), testBasicChordTiming},
        {TestCase("Multiple partial releases", "", EXPECT_PASS), testMultiplePartialReleases},
        {TestCase("Overlapping chords", "", EXPECT_PASS), testOverlappingChords},
        {TestCase("Modifier keys ignored", "", EXPECT_PASS), testModifierKeysIgnored},
        {TestCase("No chord match", "", EXPECT_PASS), testNoChordMatch},
        {TestCase("Rapid press/release", "", EXPECT_PASS), testRapidPressRelease},
        
        // Real-world scenario
        {TestCase("User reported sequence", "", EXPECT_PASS), testUserReportedSequence},
        {TestCase("Complex release pattern", "", EXPECT_PASS), testComplexReleasePattern},
        
        // Edge cases
        {TestCase("Partial chord no execution", "", EXPECT_PASS), testPartialChordNoExecution},
        {TestCase("Interrupted chord", "", EXPECT_PASS), testInterruptedChord},
    };
}

//==============================================================================
// MAIN TEST RUNNER
//==============================================================================

int main(int argc, char* argv[]) {
    bool verbose = (argc > 1 && strcmp(argv[1], "-v") == 0);
    
    std::cout << "Running Chord Timing and Release Behavior Tests" << std::endl;
    std::cout << "===============================================" << std::endl << std::endl;
    
    TestRunner runner(verbose);
    
    auto allTests = createChordTimingTests();
    
    for (const auto& testPair : allTests) {
        runner.runTest(testPair.first, testPair.second);
    }
    
    std::cout << std::endl;
    runner.printSummary();
    
    if (verbose) {
        std::cout << std::endl << "Test Details:" << std::endl;
        std::cout << "- Tests chord execution timing" << std::endl;
        std::cout << "- Verifies single execution per chord gesture" << std::endl;
        std::cout << "- Checks partial release behavior" << std::endl;
        std::cout << "- Validates modifier key handling" << std::endl;
        std::cout << "- Includes real-world usage scenarios" << std::endl;
        std::cout << "- FIXED: Proper state reset between tests" << std::endl;
    }
    
    return runner.allPassed() ? 0 : 1;
}
