/*
 * Chord State Machine Testing - Complete with All 15 Tests
 * 
 * Uses controllable time for reliable testing of timing-dependent logic
 */

#include "Arduino.h"
#include "Keyboard.h"
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

// Global variable to track state changes for debugging
static ChordState lastDebugState = CHORD_IDLE;

//==============================================================================
// TEST HELPER FUNCTIONS
//==============================================================================

void setupTestEnvironment() {
    // Clear all state
    EEPROM.clear();
    Keyboard.clearActions();
    
    // Reset time control to real time for each test (unless test overrides)
    TestTimeControl::useRealTime();
    
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
    
    // Reset chording engine state
    chording.clearAllChords();
    chording.clearAllModifiers();
    
    // Force reset by processing all keys released
    processChording(0x00);
    lastDebugState = chording.getCurrentState();
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

void addTestMacro(int keyIndex, const std::string& macroCommand) {
    if (keyIndex < 0 || keyIndex >= NUM_SWITCHES) return;
    
    std::string encoded = encodeTestMacro(macroCommand);
    if (!encoded.empty()) {
        if (macros[keyIndex].downMacro) {
            free(macros[keyIndex].downMacro);
        }
        macros[keyIndex].downMacro = (char*)malloc(encoded.length() + 1);
        strcpy(macros[keyIndex].downMacro, encoded.c_str());
    }
}

//==============================================================================
// BASIC STATE TRANSITION TESTS
//==============================================================================

void testIdleToChordBuilding(const TestCase& test) {
    setupTestEnvironment();
    
    // Add a test chord
    addTestChord(0x06, "\"hello\"");  // Keys 1+2
    
    // Start in IDLE state
    ASSERT_EQ(chording.getCurrentState(), CHORD_IDLE, "Should start in IDLE state");
    
    // Press a chord key
    bool suppressed = chording.processChording(0x02);  // Key 1
    
    ASSERT_EQ(chording.getCurrentState(), CHORD_BUILDING, "Should transition to CHORD_BUILDING");
    ASSERT_EQ(chording.getCurrentChord(), 0x02, "Should capture pressed key");
    ASSERT_TRUE(suppressed, "Should suppress individual key processing");
}

void testChordBuildingToIdle(const TestCase& test) {
    setupTestEnvironment();
    
    // Add a test chord
    addTestChord(0x06, "\"hello\"");  // Keys 1+2
    
    // Build a chord
    chording.processChording(0x02);  // Key 1
    chording.processChording(0x06);  // Keys 1+2
    
    ASSERT_EQ(chording.getCurrentState(), CHORD_BUILDING, "Should be in CHORD_BUILDING");
    
    // Release all keys
    chording.processChording(0x00);
    
    ASSERT_EQ(chording.getCurrentState(), CHORD_IDLE, "Should return to IDLE");
    ASSERT_EQ(chording.getCurrentChord(), 0, "Should clear captured chord");
}

void testChordBuildingToCancellation(const TestCase& test) {
    setupTestEnvironment();
    
    // Add a test chord and individual macro
    addTestChord(0x06, "\"hello\"");     // Keys 1+2
    addTestMacro(5, "\"world\"");        // Key 5 individual
    
    // Start building chord
    chording.processChording(0x02);  // Key 1
    ASSERT_EQ(chording.getCurrentState(), CHORD_BUILDING, "Should be in CHORD_BUILDING");
    
    // Press non-chord key
    bool suppressed = chording.processChording(0x22);  // Keys 1+5
    
    ASSERT_EQ(chording.getCurrentState(), CHORD_CANCELLATION, "Should transition to CANCELLATION");
    ASSERT_TRUE(suppressed, "Should suppress individual key processing");
}

void testCancellationToIdle(const TestCase& test) {
    setupTestEnvironment();
    
    // Setup and enter cancellation
    addTestChord(0x06, "\"hello\"");
    addTestMacro(5, "\"world\"");
    
    chording.processChording(0x02);  // Key 1
    chording.processChording(0x22);  // Keys 1+5 (triggers cancellation)
    
    ASSERT_EQ(chording.getCurrentState(), CHORD_CANCELLATION, "Should be in CANCELLATION");
    
    // Release all keys
    chording.processChording(0x00);
    
    ASSERT_EQ(chording.getCurrentState(), CHORD_IDLE, "Should return to IDLE");
}

//==============================================================================
// EXECUTION WINDOW TESTS
//==============================================================================

void testExecutionWindowTrigger(const TestCase& test) {
    setupTestEnvironment();
    
    // Add a test chord
    addTestChord(0x06, "\"hello\"");  // Keys 1+2
    
    // Build chord
    chording.processChording(0x02);  // Key 1
    chording.processChording(0x06);  // Keys 1+2
    
    ASSERT_FALSE(chording.isExecutionWindowActive(), "Execution window should not be active yet");
    
    // Release one key to trigger execution window
    chording.processChording(0x04);  // Release key 1, keep key 2
    
    ASSERT_TRUE(chording.isExecutionWindowActive(), "Execution window should be active after key release");
}

void testExecutionWindowChordExecution(const TestCase& test) {
    setupTestEnvironment();
    
    // Add a test chord
    addTestChord(0x06, "\"hello\"");  // Keys 1+2
    
    // Build and release chord quickly (within execution window)
    chording.processChording(0x02);  // Key 1
    chording.processChording(0x06);  // Keys 1+2
    chording.processChording(0x04);  // Release key 1 (start execution window)
    
    Keyboard.clearActions();
    chording.processChording(0x00);  // Release all keys within window
    
    std::string output = Keyboard.toString();
    ASSERT_TRUE(output.find("write h") != std::string::npos, "Should execute chord macro");
    ASSERT_EQ(chording.getCurrentState(), CHORD_IDLE, "Should return to IDLE after execution");
}

void testExecutionWindowPatternAdjustment(const TestCase& test) {
    setupTestEnvironment();
    
    // Use controlled time for this test
    TestTimeControl::setTime(1000);
    
    // Add chords for different patterns
    addTestChord(0x06, "\"two\"");    // Keys 1+2
    addTestChord(0x0C, "\"three\"");  // Keys 2+3
    
    // Set execution window duration
    chording.setExecutionWindowMs(50);
    
    // Build 3-key pattern
    chording.processChording(0x02);  // Key 1
    chording.processChording(0x06);  // Keys 1+2
    chording.processChording(0x0E);  // Keys 1+2+3
    
    ASSERT_EQ(chording.getCurrentChord(), 0x0E, "Should capture all three keys");
    
    // Release key 1 and start execution window
    chording.processChording(0x0C);  // Keys 2+3 remain
    ASSERT_TRUE(chording.isExecutionWindowActive(), "Execution window should be active");
    
    // Advance time past execution window
    TestTimeControl::advanceTime(60);  // 60ms > 50ms window
    
    // Process again to trigger timeout handling
    chording.processChording(0x0C);  // Same keys
    
    // The pattern should now be updated to the remaining keys (2+3)
    ASSERT_EQ(chording.getCurrentChord(), 0x0C, "Should update pattern to remaining keys 2+3");
    
    // Reset time control
    TestTimeControl::useRealTime();
}

//==============================================================================
// CANCELLATION TIMEOUT TESTS
//==============================================================================

void testCancellationTimeout(const TestCase& test) {
    setupTestEnvironment();
    
    // Use controlled time
    TestTimeControl::setTime(1000);
    
    // Setup
    addTestChord(0x06, "\"hello\"");
    addTestMacro(5, "\"world\"");
    
    // Enter cancellation state
    chording.processChording(0x02);  // Key 1
    chording.processChording(0x22);  // Keys 1+5 (cancellation)
    chording.processChording(0x02);  // Release key 5, keep key 1
    
    ASSERT_EQ(chording.getCurrentState(), CHORD_CANCELLATION, "Should be in CANCELLATION");
    
    // Advance time by 2.1 seconds (past 2-second timeout)
    TestTimeControl::advanceTime(2100);
    
    // Process again to trigger timeout
    chording.processChording(0x02);  // Same key state
    
    ASSERT_EQ(chording.getCurrentState(), CHORD_BUILDING, "Should return to CHORD_BUILDING after timeout");
    
    TestTimeControl::useRealTime();
}

void testCancellationTimeoutWithChordKeys(const TestCase& test) {
    setupTestEnvironment();
    
    // Use controlled time
    TestTimeControl::setTime(1000);
    
    addTestChord(0x06, "\"chord\"");
    addTestMacro(5, "\"individual\"");
    
    // Enter cancellation with chord keys still pressed
    chording.processChording(0x06);  // Chord keys 1+2
    chording.processChording(0x26);  // Add key 5 (cancellation)
    chording.processChording(0x06);  // Release key 5, keep chord keys
    
    ASSERT_EQ(chording.getCurrentState(), CHORD_CANCELLATION, "Should be in cancellation");
    
    // Advance past timeout
    TestTimeControl::advanceTime(2100);
    chording.processChording(0x06);  // Process with chord keys still held
    
    ASSERT_EQ(chording.getCurrentState(), CHORD_BUILDING, "Should return to CHORD_BUILDING with keys held");
    ASSERT_EQ(chording.getCurrentChord(), 0x06, "Should have correct chord pattern");
    
    TestTimeControl::useRealTime();
}

//==============================================================================
// INDIVIDUAL KEY SUPPRESSION TESTS
//==============================================================================

void testIndividualKeySuppression(const TestCase& test) {
    setupTestEnvironment();
    
    // Add individual macro
    addTestMacro(5, "\"individual\"");
    addTestChord(0x06, "\"chord\"");  // Keys 1+2
    
    // Individual key should work in IDLE state
    bool suppressed = chording.processChording(0x20);  // Key 5
    ASSERT_FALSE(suppressed, "Individual key should not be suppressed in IDLE");
    
    chording.processChording(0x00);  // Release
    
    // Individual key should be suppressed when chord keys are pressed
    chording.processChording(0x02);  // Chord key 1
    suppressed = chording.processChording(0x22);  // Add individual key 5
    ASSERT_TRUE(suppressed, "Individual key should be suppressed during chord building");
}

void testSuppressionInCancellationState(const TestCase& test) {
    setupTestEnvironment();
    
    addTestChord(0x06, "\"chord\"");
    addTestMacro(5, "\"individual\"");
    
    // Enter cancellation state
    chording.processChording(0x02);  // Chord key
    chording.processChording(0x22);  // Add individual key (triggers cancellation)
    
    ASSERT_EQ(chording.getCurrentState(), CHORD_CANCELLATION, "Should be in cancellation");
    
    // Try to press another individual key
    bool suppressed = chording.processChording(0x62);  // Add key 6
    ASSERT_TRUE(suppressed, "All keys should be suppressed in CANCELLATION state");
    ASSERT_EQ(chording.getCurrentState(), CHORD_CANCELLATION, "Should remain in CANCELLATION state");
}

//==============================================================================
// MODIFIER KEY TESTS
//==============================================================================

void testModifierKeyHandling(const TestCase& test) {
    setupTestEnvironment();
    
    // Set up modifier and chord
    chording.setModifierKey(7, true);
    addTestChord(0x86, "\"modified\"");  // Keys 1+7 (7 is modifier)
    
    // Press chord with modifier
    chording.processChording(0x02);  // Key 1
    chording.processChording(0x82);  // Keys 1+7
    
    // Release non-modifier key (key 1) but keep modifier pressed
    chording.processChording(0x80);  // Only key 7 (modifier) pressed
    
    // Should trigger chord execution even though modifier is still held
    Keyboard.clearActions();
    chording.processChording(0x00);  // Release modifier
    
    ASSERT_EQ(chording.getCurrentState(), CHORD_IDLE, "Should return to IDLE after chord");
}

void testModifierNotTriggeringCancellation(const TestCase& test) {
    setupTestEnvironment();
    
    // Set up modifier
    chording.setModifierKey(7, true);
    addTestChord(0x06, "\"chord\"");  // Keys 1+2
    
    // Start building chord
    chording.processChording(0x02);  // Key 1
    ASSERT_EQ(chording.getCurrentState(), CHORD_BUILDING, "Should be building");
    
    // Press modifier key - should not trigger cancellation
    chording.processChording(0x82);  // Keys 1+7 (modifier)
    ASSERT_EQ(chording.getCurrentState(), CHORD_BUILDING, "Modifier should not trigger cancellation");
}

//==============================================================================
// COMPLEX SCENARIO TESTS
//==============================================================================

void testComplexChordAdjustment(const TestCase& test) {
    setupTestEnvironment();
    
    // Use controlled time
    TestTimeControl::setTime(1000);
    
    // Set up multiple chord patterns
    addTestChord(0x06, "\"ab\"");     // Keys 1+2
    addTestChord(0x0E, "\"abc\"");    // Keys 1+2+3
    addTestChord(0x1E, "\"abcd\"");   // Keys 1+2+3+4
    
    chording.setExecutionWindowMs(50);
    
    // Build complex pattern and adjust down
    chording.processChording(0x02);  // Key 1
    chording.processChording(0x06);  // Keys 1+2
    chording.processChording(0x0E);  // Keys 1+2+3
    chording.processChording(0x1E);  // Keys 1+2+3+4
    
    ASSERT_EQ(chording.getCurrentChord(), 0x1E, "Should capture full pattern");
    
    // Start releasing keys
    chording.processChording(0x0E);  // Release key 4
    ASSERT_TRUE(chording.isExecutionWindowActive(), "Should start execution window");
    
    // Let execution window expire to adjust pattern
    TestTimeControl::advanceTime(60);
    chording.processChording(0x0E);  // Process with 3 keys
    
    ASSERT_EQ(chording.getCurrentChord(), 0x0E, "Should adjust to 3-key pattern");
    
    TestTimeControl::useRealTime();
}

void testCancellationRecovery(const TestCase& test) {
    setupTestEnvironment();
    
    addTestChord(0x06, "\"chord\"");
    addTestMacro(5, "\"individual\"");
    
    // Build chord, trigger cancellation, then recover
    chording.processChording(0x02);  // Chord key
    chording.processChording(0x22);  // Trigger cancellation
    
    ASSERT_EQ(chording.getCurrentState(), CHORD_CANCELLATION, "Should be in cancellation");
    
    // Release everything
    chording.processChording(0x00);
    
    ASSERT_EQ(chording.getCurrentState(), CHORD_IDLE, "Should recover to IDLE");
    
    // Should be able to start new chord immediately
    chording.processChording(0x02);
    ASSERT_EQ(chording.getCurrentState(), CHORD_BUILDING, "Should be able to start new chord");
}

//==============================================================================
// TEST CASE DEFINITIONS AND RUNNER
//==============================================================================

std::vector<std::pair<TestCase, void(*)(const TestCase&)>> createChordStateTests() {
    return {
        // Basic state transitions
        {TestCase("IDLE to CHORD_BUILDING transition", "", EXPECT_PASS), testIdleToChordBuilding},
        {TestCase("CHORD_BUILDING to IDLE transition", "", EXPECT_PASS), testChordBuildingToIdle},
        {TestCase("CHORD_BUILDING to CANCELLATION transition", "", EXPECT_PASS), testChordBuildingToCancellation},
        {TestCase("CANCELLATION to IDLE transition", "", EXPECT_PASS), testCancellationToIdle},
        
        // Execution window tests
        {TestCase("Execution window trigger", "", EXPECT_PASS), testExecutionWindowTrigger},
        {TestCase("Execution window chord execution", "", EXPECT_PASS), testExecutionWindowChordExecution},
        {TestCase("Execution window pattern adjustment", "", EXPECT_PASS), testExecutionWindowPatternAdjustment},
        
        // Cancellation timeout tests
        {TestCase("Cancellation timeout", "", EXPECT_PASS), testCancellationTimeout},
        {TestCase("Cancellation timeout with chord keys", "", EXPECT_PASS), testCancellationTimeoutWithChordKeys},
        
        // Individual key suppression
        {TestCase("Individual key suppression", "", EXPECT_PASS), testIndividualKeySuppression},
        {TestCase("Suppression in cancellation state", "", EXPECT_PASS), testSuppressionInCancellationState},
        
        // Modifier key handling
        {TestCase("Modifier key handling", "", EXPECT_PASS), testModifierKeyHandling},
        {TestCase("Modifier not triggering cancellation", "", EXPECT_PASS), testModifierNotTriggeringCancellation},
        
        // Complex scenarios
        {TestCase("Complex chord adjustment", "", EXPECT_PASS), testComplexChordAdjustment},
        {TestCase("Cancellation recovery", "", EXPECT_PASS), testCancellationRecovery},
    };
}

//==============================================================================
// MAIN TEST RUNNER
//==============================================================================

int main(int argc, char* argv[]) {
    bool verbose = (argc > 1 && strcmp(argv[1], "-v") == 0);
    
    std::cout << "Running Chord State Machine Tests" << std::endl;
    std::cout << "==================================" << std::endl << std::endl;
    
    TestRunner runner(verbose);
    
    auto allTests = createChordStateTests();
    
    for (const auto& testPair : allTests) {
        runner.runTest(testPair.first, testPair.second);
    }
    
    std::cout << std::endl;
    runner.printSummary();
    
    if (verbose) {
        std::cout << std::endl << "Test Coverage:" << std::endl;
        std::cout << "- State machine transitions (IDLE ↔ CHORD_BUILDING ↔ CANCELLATION)" << std::endl;
        std::cout << "- Execution window behavior and timing" << std::endl;
        std::cout << "- Pattern adjustment during release" << std::endl;
        std::cout << "- Individual key suppression logic" << std::endl;
        std::cout << "- Modifier key special handling" << std::endl;
        std::cout << "- Cancellation timeout handling" << std::endl;
        std::cout << "- Complex multi-step scenarios" << std::endl;
        std::cout << std::endl;
        std::cout << "Uses controllable time for reliable timeout testing" << std::endl;
    }
    
    return runner.allPassed() ? 0 : 1;
}