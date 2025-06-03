/*
 * switches-kb2040.cpp
 * 
 * KB2040-specific switch implementation using shared RP2040 base class
 */

#include "switches.h"

//==============================================================================
// GLOBAL INSTANCE AND API
//==============================================================================

// Global instance
KB2040Switches switches;

void setupSwitches() {
  switches.begin();
}

uint32_t loopSwitches() {
  return switches.update();
}