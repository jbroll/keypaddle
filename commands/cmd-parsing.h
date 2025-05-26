/*
 * Simple Serial Command Interface for UTF-8+ Key Paddle System
 * 
 * Direct command processing with minimal overhead
 */

#ifndef CMD_PARSING_H
#define CMD_PARSING_H

#define DIRECTION_UNK -1
#define DIRECTION_DOWN 0
#define DIRECTION_UP   1

//==============================================================================
// COMMAND FUNCTION TYPES
//==============================================================================

// Function type for commands that take switch, direction, and remaining args
typedef void (*SwitchDirectionCommandFunc)(int switchNum, int direction, const char* remainingArgs);

//==============================================================================
// PARSING UTILITIES
//==============================================================================

// Parse switch number and optional direction from command arguments
// Returns true on success, false on error (with error message sent to Serial)
// On success: switchNum contains the parsed switch number (0-23)
//            direction contains DIRECTION_DOWN or DIRECTION_UP
//            remainingArgs points to any remaining arguments after direction
bool parseSwitchAndDirection(const char* args, int* switchNum, int* direction, const char** remainingArgs);

// Execute a command function with parsed switch and direction
void executeWithSwitchAndDirection(const char* args, SwitchDirectionCommandFunc commandFunc);

#endif // CMD_PARSING_H
