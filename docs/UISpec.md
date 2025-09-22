# UTF-8+ Key Paddle UI Specification (Simplified)

## Core Design Philosophy
- **Single mode operation**: Mapped keys send HID, unmapped keys available for app commands
- **Two-tab interface**: Status monitoring and Configuration management
- **Hardware-first**: UI complements the standalone device

---

## Connection Management

### Device Connection Menu
- **Serial Port Selector**: Dropdown menu listing available `/dev/ttyACM*` devices
  - Auto-refresh button to rescan ports
  - Manual port entry field for custom paths
- **Connection Status**: 
  - **Disconnected**: Red indicator, "Not Connected"
  - **Connecting**: Yellow indicator, "Connecting..."
  - **Connected**: Green indicator, "Teensy 2.0 Connected on /dev/ttyACM0"
  - **Error**: Red indicator with error message
- **Connection Controls**:
  - **[Connect]** / **[Disconnect]** button
  - **[Refresh Ports]** button
  - **Auto-connect**: Checkbox to connect to first available device

---

## Tab 1: Status

### Connection & Hardware
- **Device Status**: 
  - Connection indicator with port info
  - Hardware type detection: "Teensy 2.0" / "RPi Pico" / "KB2040"
  - Firmware version if available
- **Current Switch State**: Live hex display (e.g., "Switches: 0x00FF") 
- **Active Keys**: Real-time list of currently pressed keys (e.g., "Pressed: 2, 5, 7")

### Configuration Summary
- **Individual Macros**: "12/24 keys assigned"
- **Chord Patterns**: "5 chord patterns defined"
- **Modifier Keys**: "Keys 1, 6 set as modifiers"
- **Memory Usage**: "~1.2KB free RAM"
- **Storage Status**: "Configuration loaded from EEPROM" or "Using defaults"

### Live Activity Monitor
- **Current Chord**: Shows building pattern (e.g., "Building chord: 2+3+5") 
- **Chording State**: 
  - IDLE (gray)
  - BUILDING (blue) 
  - CANCELLATION (orange)
- **Last Action**: 
  - "Executed macro: Ctrl+C" 
  - "Typed text: hello"
  - "Chord triggered: 2+3 → 'the'"
- **Activity Log**: Scrolling list of recent key events

### Quick Actions
- **[Load Config]** - Load saved configuration from device EEPROM
- **[Save Config]** - Save current configuration to device EEPROM  
- **[Clear All]** - Clear all key assignments (with confirmation dialog)
- **[Import File]** - Load configuration from file
- **[Export File]** - Save configuration to file

---

## Tab 2: Configuration

### Key Grid Display (Left Panel)

#### Visual Layout
- **Grid Representation**: Configurable layout (4x6, 6x4, 8x3, custom)
- **Key Appearance**: Rounded rectangles with clear labels
- **Responsive Sizing**: Adjustable key size and spacing

#### Color Coding System
- **Blue**: Individual macro assigned (down and/or up events)
- **Purple**: Used in chord patterns
- **Green**: Designated modifier key
- **Gray**: Unmapped (available for application commands)
- **Red outline**: Currently pressed (live hardware feedback)
- **Striped**: Key used in both individual macros AND chord patterns

#### Key Information Display
- **Key Number**: Displayed in corner (0-23)
- **Macro Preview**: First few characters of assigned macro
- **Direction Indicators**: 
  - ↓ symbol for down-event macro
  - ↑ symbol for up-event macro
  - ↕ symbol for both directions assigned
- **Chord Membership**: Small dots indicating chord pattern participation

### Configuration Editor (Right Panel)

#### Selection Information
- **Current Selection**: 
  - "Key 5" (single key)
  - "Chord Pattern: 2+3+5" (multiple keys)
  - "Modifier Keys: 1, 6" (modifier selection)
- **Assignment Status**: 
  - "Has down macro" / "Has up macro" / "Unassigned"
  - "Member of 2 chord patterns"
  - "Designated as modifier"

#### Assignment Type Selector
- **Radio Button Options**:
  - ○ Individual Key Macro
  - ○ Chord Pattern  
  - ○ Modifier Key Designation
- **Sub-options** (for Individual Key):
  - ○ Key Down Event
  - ○ Key Up Event

#### Macro Definition Editor
- **Text Area**: Large, syntax-highlighted input field
- **Current Content**: Shows existing macro definition
- **Placeholder Text**: Syntax examples based on selected type
- **Line Numbers**: For complex macros
- **Auto-completion**: Common keywords and patterns

#### Syntax Help
- **Format Examples**:
  ```
  Individual Key Examples:
  "hello world"           - Type text
  CTRL C                  - Atomic shortcut
  +SHIFT "CAPS" -SHIFT    - Hold modifier
  
  Chord Pattern Examples:
  "the"                   - Common word
  CTRL+SHIFT T            - Complex shortcut
  F1                      - Function key
  ```

#### Editor Controls
- **[Test Macro]** - Execute macro to verify behavior
- **[Clear Assignment]** - Remove current macro/assignment
- **[Copy from Key]** - Copy macro from another key
- **[Validate Syntax]** - Check macro syntax without executing

### Conflict Detection
- **Visual Warnings**: Highlight conflicting assignments in grid
- **Error Messages**: Clear descriptions of conflicts
- **Resolution Suggestions**: Recommend fixes for conflicts

### Status Bar (Bottom)
- **Configuration State**: 
  - "✓ All changes saved" (green)
  - "● Unsaved changes" (orange)
  - "⚠ Validation errors" (red)
- **Syntax Validation**: 
  - "✓ Valid syntax"
  - "⚠ Parse error: Unknown token 'CRTL'"
- **Selection Info**: "1 key selected" or "3 keys selected for chord"

---

## Interaction Model

### Connection Workflow
1. **Select Serial Port**: Choose `/dev/ttyACM0` from dropdown
2. **Connect**: Click connect button
3. **Auto-sync**: Configuration automatically loads from device
4. **Live Updates**: Status tab shows real-time device activity

### Key Selection Methods
- **Single Click**: Select individual key → shows assignment in editor
- **Ctrl+Click**: Multi-select keys → create/edit chord pattern
- **Shift+Click**: Range select keys
- **Right-Click**: Context menu (Clear, Copy, Test, etc.)

### Configuration Workflow
1. **Select Target**: Click key(s) in visual grid
2. **Choose Type**: Select assignment type (individual/chord/modifier)
3. **Define Macro**: Enter syntax in text editor
4. **Validate**: Use test button to verify behavior
5. **Save**: Save configuration to device EEPROM

### Live Feedback System
- **Real-time Grid Updates**: Pressed keys highlight immediately
- **Dynamic Editor**: Content updates when selection changes
- **Status Monitoring**: Live activity feed shows device events
- **Conflict Detection**: Immediate visual feedback for assignment conflicts

### Error Handling
- **Connection Errors**: Clear messages with troubleshooting hints
- **Syntax Errors**: Highlight problematic sections with suggestions
- **Hardware Errors**: Device communication status and recovery options
- **Validation Warnings**: Non-blocking alerts for potential issues

---

## Technical Requirements

### Serial Communication
- **Protocol**: Text-based command interface over USB serial
- **Baud Rate**: 115200 (configurable)
- **Commands**: SHOW, MAP, CHORD, SAVE, LOAD, STAT
- **Response Parsing**: Handle device responses and status updates

### Configuration Management
- **File Format**: JSON or INI format for import/export
- **Validation**: Real-time syntax checking against MAP command syntax
- **Backup**: Automatic configuration backup before major changes
- **Profiles**: Optional multiple configuration support

### Platform Support
- **Linux**: Primary target with `/dev/ttyACM*` device support
- **Cross-platform**: Extensible for Windows (COM ports) and macOS
- **Dependencies**: Minimal external libraries for serial communication

This streamlined specification provides a focused, practical interface that emphasizes the hardware's capabilities while maintaining simplicity and usability.