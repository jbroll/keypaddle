/*
 * STAT Command Implementation
 * 
 * Shows system status information
 */

#include <Arduino.h>
#include "../serial-interface.h"

int getFreeMemory() {
#ifdef ESP32
    return ESP.getFreeHeap();
#elif defined(ESP8266)
    return ESP.getFreeHeap();
#elif defined(ARDUINO_ARCH_RP2040)
    return rp2040.getFreeHeap();
#elif defined(__AVR__)
    extern char *__brkval;
    extern char __heap_start;
    return (char*)SP - (__brkval == 0 ? (char*)&__heap_start : __brkval);
#else
    return -1; // Unknown platform
#endif
}

void cmdStat() {
  Serial.print(F("Switches: 0x"));
  Serial.println(loopSwitches(), HEX);
  
  Serial.print(F("Free RAM: ~"));
  Serial.println(getFreeMemory());
}
