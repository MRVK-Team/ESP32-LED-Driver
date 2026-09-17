# ESP32-C3 Mini — Robot State Controller Implementation Specification

## 1. Purpose

Implement firmware for an **ESP32-C3 Mini** that acts as a small **robot state controller and status display**.

The ESP32 does not control robot motion directly. Its responsibility is to:

- receive robot state commands over UART,
- maintain the current robot state,
- visualize the current state on an existing **8×8 RGB LED matrix**,
- process three physical buttons,
- notify the main robot controller about button actions,
- keep the implementation simple, deterministic, and easy to extend.

Some LED patterns, matrix drawing functions, animations, button code, and visualization helpers already exist. Reuse them where practical instead of rewriting working code.

---

## 2. Main Design

The ESP32 shall behave as a finite-state machine.

```text
             UART: SET_STATE,LOAD_BEER
                         |
                         v
                 +----------------+
                 |   LOAD_BEER    |
                 +----------------+
                         |
                  Button 1 pressed
                  EVENT: BEER_LOADED
                         |
                         v
                 +----------------+
                 | AUTONOMOUS     |
                 |    DRIVING     |
                 +----------------+
                         |
                  mission finished
                         |
                         v
                 +----------------+
                 |    MISSION     |
                 |   COMPLETE     |
                 +----------------+

Any state
   |
   | UART SET_STATE,FAULT
   v
+----------------+
|     FAULT      |
+----------------+
```

The main robot controller is the authoritative source of robot state.

Buttons generate **events**. They should not silently change important robot state unless explicitly defined below.

---

# 3. Robot States

Use an enum rather than strings internally.

```cpp
enum class RobotState : uint8_t {
    LOAD_BEER = 0,
    AUTONOMOUS_DRIVING,
    FAULT,
    MISSION_COMPLETE
};
```

Recommended initial state after boot:

```text
LOAD_BEER
```

Optionally, a future version may introduce:

```text
BOOT
IDLE
MANUAL
ESTOP
CONNECTION_LOST
```

These states are not required for the first implementation.

---

# 4. State 1 — LOAD_BEER

## Meaning

The robot is waiting for a keg / beer to be loaded.

The operator must clearly understand:

> Load the beer.

## LED behavior

Use the already prepared **beer icon** as the main visualization.

Recommended animation:

```text
Beer icon ON      700 ms
Beer icon OFF     300 ms
repeat
```

Recommended color:

```text
Amber / warm yellow
```

The 8×8 display is too small for a full readable sentence at once, therefore use a combination of:

1. beer icon,
2. simple loading animation,
3. optional scrolling short text.

Suggested sequence:

```text
[BEER ICON]
     ↓
"LOAD"
     ↓
[BEER ICON]
     ↓
repeat
```

If scrolling text already exists, an alternative is:

```text
LOAD
BEER
```

However, the beer icon should remain the dominant indicator because it is immediately understandable.

## Button behavior

### Button 1 — Beer loaded

When Button 1 is pressed:

```text
EVENT: BEER_LOADED
```

Send the event to the main robot controller over UART.

Recommended behavior:

```text
Button 1
   ↓
send EVENT,BEER_LOADED
   ↓
wait for controller command:
SET_STATE,AUTONOMOUS_DRIVING
```

Do **not** immediately start robot motion directly from the ESP32.

The main controller should decide whether all other robot conditions are satisfied.

---

# 5. State 2 — AUTONOMOUS_DRIVING

## Meaning

The robot is currently operating autonomously.

The visualization should clearly communicate:

> Robot is moving / autonomous operation is active.

## LED color

Recommended color:

```text
Amber / yellow
```

This is preferable to green because green can imply that the machine is completely safe or idle.

Amber is commonly used in industrial visual signaling for warning, attention, or an active machine condition.

This project should not claim formal compliance with a specific machinery safety standard unless the complete robot signaling system has been designed and validated against that standard.

## LED animation

Recommended visualization:

```text
moving arrows
```

Example 8×8 animation:

```text
Frame 1          Frame 2          Frame 3

...##...          ....##..          .....##.
....##..          .....##.          ......##
########          .#######          ..######
....##..          .....##.          ......##
...##...          ....##..          .....##.
........          ........          ........
........          ........          ........
........          ........          ........
```

Alternative:

```text
>>> moving horizontally
```

Animation rate:

```text
100–250 ms/frame
```

Do not use very fast flashing.

Recommended:

```cpp
constexpr uint32_t AUTONOMOUS_FRAME_MS = 180;
```

---

# 6. State 3 — FAULT

## Meaning

Something is wrong with the robot.

The display must have the highest visual priority.

## LED color

```text
RED
```

## Visualization

Use a large red:

```text
X
```

or:

```text
!
```

Recommended sequence:

```text
RED X ON     500 ms
OFF          500 ms
repeat
```

Example pattern:

```text
#......#
.#....#.
..#..#..
...##...
...##...
..#..#..
.#....#.
#......#
```

Optional additional pattern:

```text
!
```

Do not overwrite the FAULT visualization because of a normal button event.

The state must remain FAULT until the main controller explicitly changes it.

---

# 7. State 4 — MISSION_COMPLETE

## Meaning

The robot successfully finished the mission.

## LED behavior

Use a cheerful animation.

Recommended possibilities:

- green check mark,
- smiley face,
- short sparkle animation,
- animated beer icon,
- alternating green check and beer icon.

Recommended sequence:

```text
GREEN CHECK
    ↓
SMILE
    ↓
BEER ICON
    ↓
repeat slowly
```

Example check:

```text
........
......#.
.....##.
#...##..
##.##...
.###....
..#.....
........
```

Recommended duration:

```text
400–700 ms/frame
```

---

# 8. Buttons

Three buttons are connected to GPIO pins.

Use:

```text
INPUT_PULLUP
```

where electrically appropriate.

Typical wiring:

```text
GPIO ---- button ---- GND
```

Pressed state:

```text
LOW
```

Configure GPIO numbers in one configuration file.

Example:

```cpp
constexpr gpio_num_t BUTTON_BEER_PIN    = GPIO_NUM_2;
constexpr gpio_num_t BUTTON_MISSION_PIN = GPIO_NUM_3;
constexpr gpio_num_t BUTTON_AUX_PIN     = GPIO_NUM_4;
```

Actual pins must be adjusted to match the PCB / ESP32-C3 Mini hardware.

---

## 8.1 Button 1 — Beer loaded

Purpose:

```text
Confirm that the keg / beer has been physically loaded.
```

UART event:

```text
EVENT,BEER_LOADED
```

Suggested response:

```text
ACK
```

The main controller can then command:

```text
SET_STATE,AUTONOMOUS_DRIVING
```

---

## 8.2 Button 2 — Mission start

Purpose:

```text
Request mission start.
```

UART event:

```text
EVENT,MISSION_START
```

This should be treated as a **request**, not permission to energize motors directly.

The main robot controller must perform safety checks before movement.

---

## 8.3 Button 3 — Placeholder

For now:

```text
EVENT,AUX_BUTTON
```

No local action is required.

This allows the hardware and communication path to be tested while keeping the button available for future use.

Potential future functions:

- cancel,
- operator acknowledgement,
- manual mode,
- return home,
- service action.

---

# 9. Button Debouncing

Implement software debouncing.

Recommended debounce time:

```cpp
constexpr uint32_t BUTTON_DEBOUNCE_MS = 40;
```

Generate only one event per physical press.

Preferred behavior:

```text
released
   ↓
pressed
   ↓
debounce
   ↓
send event once
   ↓
wait for release
```

Do not repeatedly send an event while the button is held.

Optional future addition:

```text
SHORT_PRESS
LONG_PRESS
DOUBLE_PRESS
```

Not required in the first version.

---

# 10. UART Communication

## 10.1 Purpose

UART is used between:

```text
Main Robot Controller
        ↕
     ESP32-C3
```

The protocol should be:

- human readable,
- easy to debug,
- line based,
- deterministic,
- lightweight,
- protected against corrupted messages.

Version 1 does **not** require cryptographic security.

CRC provides **error detection**, not authentication.

---

# 11. UART Electrical Configuration

Recommended initial settings:

```text
Baud rate: 115200
Data bits: 8
Parity:    none
Stop bits: 1
Flow ctrl: none
```

Equivalent:

```text
115200 8N1
```

Example:

```cpp
Serial1.begin(115200, SERIAL_8N1, RX_PIN, TX_PIN);
```

UART RX should be parsed non-blockingly.

---

# 12. Protocol Version 0 — Simplest Development Mode

During the earliest implementation, allow plain text commands.

Example:

```text
SET_STATE,LOAD_BEER
SET_STATE,AUTONOMOUS_DRIVING
SET_STATE,FAULT
SET_STATE,MISSION_COMPLETE
```

ESP response:

```text
ACK
```

Invalid command:

```text
NACK,UNKNOWN_COMMAND
```

This mode is useful for early debugging from a serial terminal.

---

# 13. Protocol Version 1 — CRC Protected

After basic communication works, switch to:

```text
$PAYLOAD*CRC\n
```

Example:

```text
$SET_STATE,LOAD_BEER*ABCD
```

Recommended CRC:

```text
CRC-16/CCITT-FALSE
```

Parameters:

```text
Polynomial: 0x1021
Initial:    0xFFFF
RefIn:      false
RefOut:     false
XorOut:     0x0000
```

Calculate CRC only over the payload:

```text
SET_STATE,LOAD_BEER
```

not over:

```text
$
*
CRC
newline
```

---

# 14. UART Message Format

General frame:

```text
$<COMMAND>[,<ARG1>,<ARG2>...]*<CRC16>\n
```

Example:

```text
$SET_STATE,AUTONOMOUS_DRIVING*4A21
```

Responses:

```text
$ACK,SET_STATE*XXXX
```

or:

```text
$NACK,BAD_CRC*XXXX
```

---

# 15. Required Incoming Commands

## SET_STATE

```text
SET_STATE,<state>
```

Examples:

```text
SET_STATE,LOAD_BEER
SET_STATE,AUTONOMOUS_DRIVING
SET_STATE,FAULT
SET_STATE,MISSION_COMPLETE
```

On success:

```text
ACK,SET_STATE
```

---

## GET_STATE

Command:

```text
GET_STATE
```

Response:

```text
STATE,LOAD_BEER
```

or the current state.

---

## PING

Command:

```text
PING
```

Response:

```text
PONG
```

Useful for:

- startup detection,
- UART testing,
- controller health checks.

---

## SET_BRIGHTNESS

Optional but recommended.

```text
SET_BRIGHTNESS,0-255
```

Example:

```text
SET_BRIGHTNESS,80
```

Store only in RAM initially.

---

# 16. Outgoing Events

The ESP32 can send asynchronous events.

Required:

```text
EVENT,BEER_LOADED
EVENT,MISSION_START
EVENT,AUX_BUTTON
```

Example CRC framed event:

```text
$EVENT,BEER_LOADED*XXXX
```

---

# 17. UART Parser

Do not use blocking functions such as:

```cpp
readStringUntil()
delay()
```

inside the main logic.

Use a receive buffer.

Pseudo implementation:

```cpp
void processUartByte(char c)
{
    if (c == '\n') {
        parseFrame(rxBuffer);
        rxBuffer.clear();
        return;
    }

    if (rxBuffer.size() < MAX_FRAME_SIZE) {
        rxBuffer += c;
    } else {
        rxBuffer.clear();
        sendNack("FRAME_TOO_LONG");
    }
}
```

Recommended maximum frame size:

```cpp
constexpr size_t MAX_FRAME_SIZE = 96;
```

---

# 18. CRC Validation Flow

```text
UART frame received
       |
       v
check start '$'
       |
       v
find '*'
       |
       v
extract payload
       |
       v
parse received CRC
       |
       v
calculate CRC(payload)
       |
       +---- mismatch ----> NACK,BAD_CRC
       |
       v
parse command
       |
       v
execute
       |
       v
ACK
```

---

# 19. Future Security Upgrade

CRC protects against accidental transmission errors.

It does **not** protect against malicious commands.

A future secure protocol may add:

```text
sequence number
device ID
message authentication code
```

Possible future frame:

```text
$SEQ,DEVICE_ID,COMMAND,ARGS*CRC*AUTH
```

For example:

```text
$42,ROBOT_LED_01,SET_STATE,FAULT*CRC*MAC
```

Possible authentication approaches:

```text
HMAC-SHA256
AES-CMAC
```

A truncated MAC can be used if bandwidth becomes important.

Do not implement cryptographic authentication in the first version unless required.

Design the parser so the framing can be extended later.

---

# 20. State Manager

Create one centralized state manager.

Example:

```cpp
class StateManager {
public:
    void begin();
    void update();

    void setState(RobotState newState);
    RobotState getState() const;

private:
    RobotState currentState = RobotState::LOAD_BEER;
    uint32_t stateEnteredAt = 0;
};
```

Changing the state should occur only through:

```cpp
setState()
```

This makes debugging easier.

---

# 21. LED Animation Manager

Animations should be independent from UART parsing.

Recommended interface:

```cpp
class LedAnimator {
public:
    void begin();
    void update(uint32_t nowMs);
    void setState(RobotState state);
    void setBrightness(uint8_t brightness);

private:
    RobotState activeState;
    uint32_t lastFrameTime = 0;
    uint8_t frameIndex = 0;
};
```

Main loop:

```cpp
void loop()
{
    const uint32_t now = millis();

    uartManager.update();
    buttonManager.update(now);
    stateManager.update();
    ledAnimator.update(now);
}
```

Do not place long delays in animations.

---

# 22. Existing LED Patterns

Reuse already implemented:

- beer logo,
- matrix drawing functions,
- existing patterns,
- brightness handling,
- animation helpers.

Refactor them into reusable functions.

Example:

```cpp
void drawBeerIcon();
void drawFaultIcon();
void drawCheckIcon();
void drawDrivingFrame(uint8_t frame);
```

Avoid mixing UART parsing directly with LED pixel drawing.

Bad:

```cpp
if (command == "FAULT") {
    matrix.setPixel(...);
    matrix.setPixel(...);
    ...
}
```

Preferred:

```cpp
stateManager.setState(RobotState::FAULT);
```

Then:

```cpp
ledAnimator.update();
```

handles visualization.

---

# 23. Suggested Source Structure

```text
src/
├── main.cpp
├── config.h
│
├── state/
│   ├── robot_state.h
│   ├── state_manager.h
│   └── state_manager.cpp
│
├── uart/
│   ├── uart_protocol.h
│   ├── uart_protocol.cpp
│   ├── crc16.h
│   └── crc16.cpp
│
├── buttons/
│   ├── button_manager.h
│   └── button_manager.cpp
│
└── display/
    ├── led_animator.h
    ├── led_animator.cpp
    ├── icons.h
    └── icons.cpp
```

For a very small prototype, fewer files are acceptable, but keep the logical separation.

---

# 24. Configuration File

Put hardware-specific settings in one location.

Example:

```cpp
#pragma once

// UART
constexpr int UART_RX_PIN = 20;
constexpr int UART_TX_PIN = 21;
constexpr uint32_t UART_BAUD = 115200;

// Buttons
constexpr int BUTTON_BEER_PIN = 2;
constexpr int BUTTON_MISSION_PIN = 3;
constexpr int BUTTON_AUX_PIN = 4;

// LED
constexpr int LED_DATA_PIN = 5;
constexpr uint8_t LED_BRIGHTNESS_DEFAULT = 80;

// Timing
constexpr uint32_t BUTTON_DEBOUNCE_MS = 40;
constexpr uint32_t AUTONOMOUS_FRAME_MS = 180;
constexpr uint32_t FAULT_BLINK_MS = 500;
```

These values are examples only.

Do not assume these GPIO assignments are correct for the final PCB.

---

# 25. Main Program Flow

```cpp
void setup()
{
    uartManager.begin();
    buttonManager.begin();
    stateManager.begin();
    ledAnimator.begin();

    stateManager.setState(RobotState::LOAD_BEER);
}

void loop()
{
    const uint32_t now = millis();

    uartManager.update();

    buttonManager.update(now);

    stateManager.update();

    ledAnimator.setState(stateManager.getState());
    ledAnimator.update(now);
}
```

---

# 26. Command Handling Example

Pseudo implementation:

```cpp
void handleCommand(const Command& command)
{
    if (command.name == "PING") {
        sendMessage("PONG");
        return;
    }

    if (command.name == "GET_STATE") {
        sendState();
        return;
    }

    if (command.name == "SET_STATE") {
        RobotState state;

        if (!parseRobotState(command.arg(0), state)) {
            sendNack("BAD_STATE");
            return;
        }

        stateManager.setState(state);
        sendAck("SET_STATE");
        return;
    }

    if (command.name == "SET_BRIGHTNESS") {
        // validate 0..255
        // update animator
        // ACK
        return;
    }

    sendNack("UNKNOWN_COMMAND");
}
```

---

# 27. Invalid Message Handling

The ESP32 must never crash because of malformed UART data.

Required error responses may include:

```text
NACK,BAD_CRC
NACK,BAD_FORMAT
NACK,UNKNOWN_COMMAND
NACK,BAD_STATE
NACK,BAD_ARGUMENT
NACK,FRAME_TOO_LONG
```

Ignore random bytes before:

```text
$
```

Recover automatically after malformed frames.

---

# 28. State Priority

Recommended logical priority:

```text
FAULT
MISSION_COMPLETE
AUTONOMOUS_DRIVING
LOAD_BEER
```

However, because the main controller explicitly commands the state, normally only one state exists at a time.

If a future implementation introduces simultaneous alerts, FAULT must override all normal animations.

---

# 29. Startup Behavior

On boot:

1. initialize LED matrix,
2. initialize buttons,
3. initialize UART,
4. optionally show a short boot animation,
5. enter `LOAD_BEER`,
6. send:

```text
BOOT,READY
```

Example framed:

```text
$BOOT,READY*XXXX
```

The robot controller may then query:

```text
GET_STATE
```

---

# 30. Communication Watchdog

Recommended, but optional for version 1.

Record the last valid UART message:

```cpp
uint32_t lastValidMessageMs;
```

Possible future behavior:

```text
No valid controller message for 5 s
        ↓
communication warning
```

Do **not** automatically stop or control the robot motors from this ESP unless the system architecture explicitly assigns it safety authority.

This controller is primarily a user-interface/status device.

---

# 31. Expected User Flow

## Loading

```text
Robot controller:
SET_STATE,LOAD_BEER

ESP:
beer icon + LOAD animation
```

Operator loads beer.

Operator presses Button 1.

```text
ESP -> controller:
EVENT,BEER_LOADED
```

Controller validates everything.

```text
Controller -> ESP:
SET_STATE,AUTONOMOUS_DRIVING
```

ESP:

```text
amber moving-arrow animation
```

---

## Mission start button

Operator presses Button 2.

ESP:

```text
EVENT,MISSION_START
```

Main controller decides whether the mission can begin.

---

## Fault

Controller detects a fault.

```text
SET_STATE,FAULT
```

ESP immediately displays:

```text
RED X flashing
```

---

## Mission finished

Controller sends:

```text
SET_STATE,MISSION_COMPLETE
```

ESP displays:

```text
green check / smile / beer celebration
```

---

# 32. Functional Requirements

The implementation is complete when all of the following work.

### State control

- ESP starts in `LOAD_BEER`.
- State can be changed via UART.
- `GET_STATE` returns the actual current state.
- Unknown state names are rejected.
- Repeated state commands do not crash or reset the ESP.

### Buttons

- Button 1 sends `EVENT,BEER_LOADED`.
- Button 2 sends `EVENT,MISSION_START`.
- Button 3 sends `EVENT,AUX_BUTTON`.
- Each press generates only one event.
- Holding a button does not continuously generate events.

### LED matrix

- `LOAD_BEER` shows the beer/load visualization.
- `AUTONOMOUS_DRIVING` shows animated amber driving indication.
- `FAULT` shows flashing red fault indication.
- `MISSION_COMPLETE` shows a cheerful successful-completion animation.
- Animations do not block UART reception.

### UART

- `PING` returns `PONG`.
- `SET_STATE` works.
- `GET_STATE` works.
- malformed messages are safely rejected.
- CRC failure is detected in protocol v1.
- the UART receiver never blocks the main loop.

---

# 33. Testing Procedure

## Test 1 — Boot

Power the ESP32.

Expected:

```text
beer/load state shown
BOOT,READY transmitted
```

---

## Test 2 — UART ping

Send:

```text
PING
```

Expected:

```text
PONG
```

---

## Test 3 — Load state

Send:

```text
SET_STATE,LOAD_BEER
```

Expected:

```text
beer icon + loading indication
```

---

## Test 4 — Beer button

Press Button 1.

Expected UART event:

```text
EVENT,BEER_LOADED
```

Exactly once.

---

## Test 5 — Autonomous driving

Send:

```text
SET_STATE,AUTONOMOUS_DRIVING
```

Expected:

```text
amber moving animation
```

---

## Test 6 — Mission start

Press Button 2.

Expected:

```text
EVENT,MISSION_START
```

---

## Test 7 — Fault

Send:

```text
SET_STATE,FAULT
```

Expected:

```text
flashing red X
```

---

## Test 8 — Mission complete

Send:

```text
SET_STATE,MISSION_COMPLETE
```

Expected:

```text
green/check/celebration animation
```

---

## Test 9 — Invalid command

Send:

```text
ABC
```

Expected:

```text
NACK,UNKNOWN_COMMAND
```

or:

```text
NACK,BAD_FORMAT
```

depending on parser stage.

---

## Test 10 — Invalid CRC

Send a correctly formatted message with deliberately incorrect CRC.

Expected:

```text
NACK,BAD_CRC
```

State must remain unchanged.

---

# 34. Implementation Order for the Coding Agent

Implement in this order:

1. Preserve existing working LED patterns and matrix code.
2. Create `RobotState` enum.
3. Add centralized `StateManager`.
4. Refactor existing LED behavior into `LedAnimator`.
5. Implement `LOAD_BEER`.
6. Implement `AUTONOMOUS_DRIVING`.
7. Implement `FAULT`.
8. Implement `MISSION_COMPLETE`.
9. Refactor three buttons into `ButtonManager`.
10. Add non-blocking button debounce.
11. Implement UART protocol v0 using plain lines.
12. Implement:
    - `PING`
    - `GET_STATE`
    - `SET_STATE`
    - button events.
13. Verify all functionality.
14. Add CRC-16/CCITT-FALSE framing.
15. Add protocol error handling.
16. Add optional `SET_BRIGHTNESS`.
17. Remove unused legacy logic only after all previous functionality passes testing.

---

# 35. Important Constraints for the Agent

Do not:

- rewrite working LED patterns unnecessarily,
- use long blocking `delay()` calls,
- allow UART parsing to block animations,
- allow LED animations to block button handling,
- hard-code state logic in multiple files,
- start motors directly from button presses,
- treat CRC as cryptographic security,
- silently accept unknown state names,
- continuously emit button events while a button is held.

Prefer:

- `millis()` based timing,
- enums,
- centralized state transitions,
- small testable modules,
- explicit ACK/NACK responses,
- deterministic UART messages,
- clear serial debug logs during development.

---

# 36. Minimal First Milestone

The first usable firmware milestone only needs:

```text
4 robot states
3 buttons
plain UART commands
8×8 visualization
non-blocking loop
```

Commands:

```text
PING
GET_STATE
SET_STATE,LOAD_BEER
SET_STATE,AUTONOMOUS_DRIVING
SET_STATE,FAULT
SET_STATE,MISSION_COMPLETE
```

Events:

```text
EVENT,BEER_LOADED
EVENT,MISSION_START
EVENT,AUX_BUTTON
```

After this version works reliably, add:

```text
$PAYLOAD*CRC16
```

framing.

This staged approach keeps debugging simple and avoids introducing protocol complexity before the state machine and hardware behavior are verified.
