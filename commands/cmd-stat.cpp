/*
 * STAT Command Implementation
 * 
 * Shows system status information
 */

#include "../serial-interface.h"

void cmdStat() {
  Serial.print(F("Switches: 0x"));
  Serial.println(loopSwitches(), HEX);
  
  // Free RAM estimate
  extern int __heap_start, *__brkval;
  int v;
  Serial.print(F("Free RAM: ~"));
  Serial.println((int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval));
}