# CmdLib

A lightweight, cross-platform command **parser & builder** for structured command strings used by microcontrollers and host software.

---

## Format

Valid command forms:

**Direct to router (from device):**
```
!!DESTINATION:MSG_KIND:COMMAND{key=value,...}##
```

**Forwarded by router (to device):**
```
!!SOURCE:DESTINATION:MSG_KIND:COMMAND{key=value,...}##
```

**Field definitions:**
- `DESTINATION` — the intended recipient of the command
- `SOURCE` *(optional, added by router)* — the originating sender (prepended by router when forwarding)
- `MSG_KIND` *(required)* — message category (`REQUEST`, `CONFIRM`, `ERROR`, ...).  
- `COMMAND` *(required)* — the command name (`MAKE_STAR`, `BUILDUP_CLIMAX_CENTER`, ...).  
- `{key=value,...}` *(optional)* — named parameters. When no parameters are present the braces may be omitted, e.g. `!!ARM1:REQUEST:MAKE_STAR##`.

**Important:** All parameters inside `{}` **must** be `key=value` pairs. Positional parameters are not supported.

**Router behavior:** When the router receives a message, it prepends the sender's identity as the first header, converting `!!TO:MSGTYPE:COMMAND##` into `!!FROM:TO:MSGTYPE:COMMAND##` before forwarding to the destination.

---

## Features

- Cross-platform: works on **Arduino** (uses `String`) and **standard C++** (uses `std::string`)
- Automatic Arduino mode detection when `ARDUINO` is defined (or define `CMDLIB_ARDUINO`)
- Small footprint: Arduino build uses fixed arrays (no dynamic allocation)
- Simple API: build commands programmatically and serialize with `toString()`; parse strings with `parse()`
- All parameters are **named** (`key=value`) — consistent and explicit
- Multi-level header routing for complex communication topologies

---

## Message Flow & Header Routing

In a distributed system with a router intermediary, commands flow through multiple hops. **The router adds the sender as the first header** when forwarding, converting `!!TO:MSGTYPE:COMMAND##` to `!!FROM:TO:MSGTYPE:COMMAND##`

### Example: MASTER → ARM1 via ROUTER

**Step 1: MASTER sends to ROUTER**

MASTER sends directly to the router with only the destination:
```
!!ARM1:REQUEST:MAKE_STAR{size=120,color=RED,speed=100,count=6}##
  ↑
  TO (destination only - MASTER is implicit sender)
```

- Format: `!!TO:MSGTYPE:COMMAND##`
- ROUTER receives this and knows MASTER sent it (implicit from connection)

**Step 2: ROUTER forwards to ARM1**

ROUTER prepends itself as the FROM header and sends:
```
!!MASTER:ARM1:REQUEST:MAKE_STAR{size=120,color=RED,speed=100,count=6}##
  ↑      ↑
  FROM   TO
```

- Format: `!!FROM:TO:MSGTYPE:COMMAND##` (router added MASTER as FROM)
- ARM1 receives and sees the original sender (MASTER) in `headers[0]`
- ARM1 sees the command was routed from MASTER

**Step 3: ARM1 responds to ROUTER**

ARM1 sends back with the original sender as destination:
```
!!MASTER:CONFIRM:MAKE_STAR{size=120,color=RED,speed=100,count=6}##
  ↑
  TO (destination only - ARM1 is implicit sender)
```

- Format: `!!TO:MSGTYPE:COMMAND##`
- ROUTER receives this from ARM1 and knows ARM1 is the sender

**Step 4: ROUTER forwards response back to MASTER**

ROUTER prepends ARM1 as the FROM header:
```
!!ARM1:MASTER:CONFIRM:MAKE_STAR{size=120,color=RED,speed=100,count=6}##
  ↑    ↑
  FROM TO
```

- Format: `!!FROM:TO:MSGTYPE:COMMAND##` (router added ARM1 as FROM)
- MASTER receives and sees the response came from ARM1

### Header Pattern Summary

**Devices sending TO router (direct connection):**
```
!!TO:MSGTYPE:COMMAND##
```
Only specifies the destination; sender is implicit.

**Router forwarding between devices:**
```
!!FROM:TO:MSGTYPE:COMMAND##
```
Router inserts the source as the first header to maintain communication context.

| Step | Sender | Format | Message |
|------|--------|--------|---------|
| 1 | MASTER → ROUTER | `!!ARM1:REQUEST:MAKE_STAR##` | Send to ARM1 |
| 2 | ROUTER → ARM1 | `!!MASTER:ARM1:REQUEST:MAKE_STAR##` | From MASTER, to ARM1 |
| 3 | ARM1 → ROUTER | `!!MASTER:CONFIRM:MAKE_STAR##` | Send to MASTER |
| 4 | ROUTER → MASTER | `!!ARM1:MASTER:CONFIRM:MAKE_STAR##` | From ARM1, to MASTER |

**Key insight:** The router acts as a bridge that converts single-header messages (`!!TO:...##`) into two-header messages (`!!FROM:TO:...##`) by prepending the sender's identity. This allows both direct point-to-point communication through the router and proper message tracing.

### ADD_STAR Command

Request to add stars to the display with speed, color, brightness, and size:

```
!!MASTER:REQUEST:ADD_STAR{count=5,speed=50,color=0xffc003,brightness=255,size=1}##
```

- `MASTER` — destination header
- `REQUEST` — message kind
- `ADD_STAR` — command name
- Parameters: `count`, `speed`, `color`, `brightness`, `size`

Response (on success):

```
!!MASTER:CONFIRM:ADD_STAR##
```

### BUILDUP_CLIMAX_CENTER Command

Request to start a climax buildup animation with duration and speed multiplier:

```
!!MASTER:REQUEST:BUILDUP_CLIMAX_CENTER{duration=10.0,speedMultiplier=5.0}##
```

### START_CLIMAX_CENTER Command

Request to start the climax spiral animation:

```
!!MASTER:REQUEST:START_CLIMAX_CENTER{duration=15.0,spiralSpeed=0.5,speedMultiplier=5.0,verticalBias=1.2}##
```

### PING/PONG Keep-Alive

Simple keep-alive request:

```
!!MASTER:REQUEST:PING##
```

Response:

```
!!MASTER:CONFIRM:PING##
```

### Error Response

When a command fails validation:

```
!!MASTER:ERROR:ADD_STAR{message=Speed must be between 0 and 100, got: 150}##
```

---

## API & Usage

The library exposes a `Command` struct and a `parse()` function in the `cmdlib` namespace.

### C++ (STL) example

```cpp
#include <iostream>
#include "CmdLib.h"

int main() {
  std::string in = "!!MASTER:REQUEST:ADD_STAR{count=5,speed=50,color=0xffc003,brightness=255,size=1}##";
  cmdlib::Command cmd;
  std::string err;

  if (cmdlib::parse(in, cmd, err)) {
    std::cout << "msgKind: " << cmd.msgKind << "\n";           // "REQUEST"
    std::cout << "command: " << cmd.command << "\n";           // "ADD_STAR"
    std::cout << "count: " << cmd.getNamed("count") << "\n";   // "5"
    std::cout << "speed: " << cmd.getNamed("speed") << "\n";   // "50"
    std::cout << "header: " << cmd.headers[0] << "\n";         // "MASTER"
  } else {
    std::cerr << "Parse error: " << err << "\n";
  }

  // Build a command:
  cmdlib::Command out;
  out.addHeader("MASTER");
  out.msgKind = "CONFIRM";
  out.command = "ADD_STAR";
  out.setNamed("count", "3");
  std::string s = out.toString();
  // s: !!MASTER:CONFIRM:ADD_STAR{count=3}##
}
```

### Arduino example

```cpp
#include "CmdLib.h"

void setup() {
  Serial.begin(115200);
  
  cmdlib::Command cmd;
  String err;
  String in = "!!MASTER:REQUEST:ADD_STAR{count=5,speed=50,color=0xffc003}##";
  
  if (cmdlib::parse(in, cmd, err)) {
    Serial.println(cmd.msgKind);                    // "REQUEST"
    Serial.println(cmd.command);                    // "ADD_STAR"
    Serial.println(cmd.getNamed("count"));          // "5"
    Serial.println(cmd.getNamed("speed", "0"));     // "50"
    Serial.println(cmd.getNamed("brightness", "0")); // "0" (default)
  } else {
    Serial.print("Parse error: ");
    Serial.println(err);
  }

  // Build and send a response:
  cmdlib::Command response;
  response.addHeader("MASTER");
  response.msgKind = "CONFIRM";
  response.command = "ADD_STAR";
  response.setNamed("added", "5");
  Serial.println(response.toString()); 
  // Output: !!MASTER:CONFIRM:ADD_STAR{added=5}##
}

void loop() { }
```

---

## Header Routing

The header structure depends on whether a message is being sent directly to the router or being forwarded by the router:

**Direct message to router (from device):**
```
!!DESTINATION:MSGTYPE:COMMAND##
```
Only one header—the intended destination. The sender is implicit (known by the router from the connection).

**Forwarded message from router (to device):**
```
!!SOURCE:DESTINATION:MSGTYPE:COMMAND##
```
Two headers—the source (added by router) and the destination. This provides full context about where the message originated.

**Accessing headers in code:**
```cpp
// When receiving from router with both headers
String source = cmd.getHeader(0);         // Who sent it (added by router)
String destination = cmd.getHeader(1);    // Where it's going

// When building a response back through router
response.addHeader(cmd.getHeader(0));     // Echo the source as destination
```

---

## Parameter Types & Defaults

All parameters are stored as strings internally. Use type conversion methods as needed:

```cpp
// String parameters
String color = cmd.getNamed("color", "0xffffff");

// Integer parameters (use .toInt())
int count = cmd.getNamed("count", "1").toInt();
int speed = cmd.getNamed("speed", "50").toInt();

// Float parameters (use .toFloat())
float duration = cmd.getNamed("duration", "10.0").toFloat();

// Boolean parameters
bool flag = (cmd.getNamed("flag", "") != "");
```

---

## Error Handling & Validation

The parser validates and returns an error message when:

- Missing prefix `!!` or missing suffix `##`
- Unbalanced or malformed braces `{}` (if present)
- Empty header (no routing information)
- Incomplete header (missing both msgKind and command)
- Stray closing brace without opening brace
- Empty parameter key (e.g., `=value`)

**Example error handling:**

```cpp
cmdlib::Command cmd;
String error;

if (!cmdlib::parse(incoming, cmd, error)) {
  Serial.print("Parse failed: ");
  Serial.println(error);  // e.g., "Missing prefix '!!'"
  // Send error response
  cmdlib::Command errResp;
  errResp.addHeader("MASTER");
  errResp.msgKind = "ERROR";
  errResp.command = cmd.command;
  errResp.setNamed("message", error);
  Serial.println(errResp.toString());
  return;
}
```

---

## Configuration

### Arduino Mode
- Automatically enabled when `ARDUINO` is defined.
- You may explicitly force Arduino mode by defining `CMDLIB_ARDUINO` prior to including the header.

### Max Parameters (Arduino only)
- Default maximum: **12** parameters.
- Override by defining `CMDLIB_MAX_PARAMS` before including:

```cpp
#define CMDLIB_MAX_PARAMS 20
#include "CmdLib.h"
```

### Max Header Parts (Arduino only)
- Default maximum: **8** header parts.
- Override by defining `CMDLIB_MAX_HEADER_PARTS` before including:

```cpp
#define CMDLIB_MAX_HEADER_PARTS 16
#include "CmdLib.h"
```

---

## Unit Tests

A C++17 test harness exercises the standard-library build (multi-header support, round-trip serialization, flag parameters, error detection, and clear/overwrite semantics).

```bash
g++ -std=c++17 CmdLibTest.cpp -o CmdLibTest
./CmdLibTest
```

Expected output:

```
[PASS] build round-trip
[PASS] parse with flags
[PASS] error detection
[PASS] clear and overwrite
[PASS] multi-header parse

Summary: 5 passed, 0 failed
```

---

## Handler Pattern (Command Dispatcher)

A common pattern is to use handler classes to dispatch commands based on type:

```cpp
#include "base_command_handler.h"

class MyCommandHandler : public BaseCommandHandler {
public:
    bool canHandle(const String &command) const override {
        return command == "MY_COMMAND";
    }
    
    String getName() const override {
        return "MyHandler";
    }
    
    void handle(const cmdlib::Command &cmd, cmdlib::Command &response) override {
        // Validate parameters
        int count = cmd.getNamed("count", "1").toInt();
        if (count <= 0) {
            buildError(response, cmd.command, "Count must be positive", cmd.getHeader(0));
            return;
        }
        
        // Process command
        // ... do work ...
        
        // Send response
        buildResponse(response, cmd.command, cmd.getHeader(0));
    }
};
```

The base class provides helper methods for building responses:

- `buildResponse()` — create a CONFIRM message
- `buildRequest()` — create a REQUEST message
- `buildError()` — create an ERROR message with a message field

---

## API Reference (Summary)

### Struct `cmdlib::Command`

**Fields:**
- `std::vector<string> headers` (C++) or `String headers[CMDLIB_MAX_HEADER_PARTS]` (Arduino)
- `string msgKind` — message kind (e.g., "REQUEST", "CONFIRM", "ERROR")
- `string command` — command name

**Methods:**
- `void addHeader(const string &h)` / `bool addHeader(const String &h)` — add a header part
- `string getHeader(size_t i)` / `String getHeader(int i)` — retrieve header at index
- `void setNamed(const string &k, const string &v)` / `bool setNamed(const String &k, const String &v)` — set or update a parameter
- `string getNamed(const string &k, const string &def = "")` / `String getNamed(const String &k, const String &def = "")` — get parameter value with optional default
- `string toString()` / `String toString()` — serialize to command string
- `void clear()` — reset all fields

### Function

**`bool parse(const string &input, Command &out, string &error)`** (C++) or **`bool parse(const String &input, Command &out, String &error)`** (Arduino)

- Parses `input` string into `out` Command object
- Returns `true` on success, `false` on error
- Error message is populated on failure

---

## Common Patterns

### Parsing with Default Values

```cpp
int speed = cmd.getNamed("speed", "50").toInt();
float duration = cmd.getNamed("duration", "10.0").toFloat();
String color = cmd.getNamed("color", "0xffffff");
int brightness = cmd.getNamed("brightness", "255").toInt();
```

### Building a Response

```cpp
cmdlib::Command response;
response.addHeader(cmd.getHeader(0));  // Echo source as destination
response.msgKind = "CONFIRM";
response.command = cmd.command;
response.setNamed("status", "success");
response.setNamed("added", "5");
Serial.println(response.toString());
```

### Building an Error

```cpp
cmdlib::Command errResp;
errResp.addHeader(cmd.getHeader(0));
errResp.msgKind = "ERROR";
errResp.command = cmd.command;
errResp.setNamed("message", "Invalid parameter value");
Serial.println(errResp.toString());
```

---

## License

**Timo's Cookie License** — usage cost: **1 cookie**.  
