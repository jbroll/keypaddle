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
  Serial.println((int)((intptr_t)&v - (__brkval == 0 ? (intptr_t)&__heap_start : (intptr_t)__brkval)));
}