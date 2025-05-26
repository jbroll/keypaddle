/*
 * Non-blocking Serial Readline Implementation
 * 
 * Character-by-character input with basic line editing
 */

//==============================================================================
// CONFIGURATION
//==============================================================================

#define MAX_CMD_LINE 128

//==============================================================================
// COMMAND BUFFER
//==============================================================================

static char commandBuffer[MAX_CMD_LINE];
static int bufferPos = 0;

//==============================================================================
// NON-BLOCKING READLINE IMPLEMENTATION
//==============================================================================

// Returns pointer to completed line or nullptr if still reading
const char* readLine() {
  while (Serial.available()) {
    char c = Serial.read();
    
    if (c == '\n' || c == '\r') {
      if (bufferPos > 0) {
        commandBuffer[bufferPos] = '\0';  // Null terminate
        bufferPos = 0;
        return commandBuffer;
      }
    }
    else if (c == '\b' || c == 127) {  // Backspace or DEL
      if (bufferPos > 0) {
        bufferPos--;
        Serial.print(F("\b \b"));  // Erase character on terminal
      }
    }
    else if (c >= 32 && c <= 126 && bufferPos < MAX_CMD_LINE - 1) {  // Printable chars
      commandBuffer[bufferPos++] = c;
      Serial.print(c);  // Echo character
    }
  }
  return nullptr;  // Still reading
}
