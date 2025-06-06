/*
 * switches-pico.cpp
 * 
 * RPi Pico-specific switch implementation using shared RP2040 base class
 */

#include "switches.h"

//==============================================================================
// GLOBAL INSTANCE AND API
//==============================================================================

// Global instance
PicoSwitches switches;

void setupSwitches() {
  switches.begin();
}

uint32_t loopSwitches() {
  return switches.update();
}