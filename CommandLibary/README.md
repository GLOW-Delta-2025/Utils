# CmdLib

A lightweight, cross-platform command **parser & builder** for structured command strings used by microcontrollers and host software.

---

## Format

Valid command form:

```
!![FROM[:TO[:...]]]:MSG_KIND:COMMAND{key=value,key2=value2}##
```

- `FROM` — the originating sender (e.g., `MASTER`, `ARM1`)
- `TO` — the intended recipient(s). Can be a single destination or a chain of routing hops
- Additional headers — optional intermediate routing nodes  
- `MSG_KIND` *(required)* — message category (`REQUEST`, `CONFIRM`, `ERROR`, ...).  
- `COMMAND` *(required)* — the command name (`MAKE_STAR`, `BUILDUP_CLIMAX_CENTER`, ...).  
- `{key=value,...}` *(optional)* — named parameters. When no parameters are present the braces may be omitted, e.g. `!!CONFIRM:MAKE_STAR##`.

**Important:** All parameters inside `{}` **must** be `key=value` pairs. Positional parameters are not supported.

**Header Convention:** The header format is `!!FROM:TO:MSGTYPE:COMMAND##` where the router adds itself as an intermediate hop when forwarding.

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

In a distributed system with a router intermediary, commands flow through multiple hops. The header chain is **reordered at each hop** to maintain the pattern: `!!FROM:TO:MSGTYPE:COMMAND##`

### Example: MASTER → ARM1 via ROUTER

**Step 1: MASTER sends to ARM1 (through ROUTER)**

MASTER sends:
```
!!MASTER:ARM1:REQUEST:MAKE_STAR{size=120,color=RED,speed=100,count=6}##
        ↑     ↑
      FROM   TO
```

- `FROM` = MASTER (originator)
- `TO` = ARM1 (intended recipient)
- The ROUTER receives this and understands it needs to forward to ARM1

**Step 2: ROUTER forwards to ARM1**

ROUTER inserts itself as an intermediate hop and reorders headers:
```
!!MASTER:ARM1:REQUEST:MAKE_STAR{size=120,color=RED,speed=100,count=6}##
        ↑    ↑
      FROM  TO (unchanged, ARM1 still the target)
```

Actually receives (with original sender preserved):
```
!!MASTER:ARM1:REQUEST:MAKE_STAR{size=120,color=RED,speed=100,count=6}##
```

ARM1 sees:
- `headers[0]` = MASTER (original source)
- `headers[1]` = ARM1 (itself, confirmed as destination)
- Understands the message originated from MASTER

**Step 3: ARM1 responds with CONFIRM**

ARM1 sends back:
```
!!ARM1:MASTER:CONFIRM:MAKE_STAR{size=120,color=RED,speed=100,count=6}##
       ↑     ↑
      FROM  TO (reversed: now sending TO the original MASTER)
```

- `FROM` = ARM1 (now the sender)
- `TO` = MASTER (the original sender becomes the recipient)

**Step 4: ROUTER forwards response back to MASTER**

ROUTER sees ARM1→MASTER and routes accordingly:
```
!!ARM1:MASTER:CONFIRM:MAKE_STAR{size=120,color=RED,speed=100,count=6}##
       ↑     ↑
      FROM  TO
```

MASTER receives and sees:
- `headers[0]` = ARM1 (response from ARM1)
- `headers[1]` = MASTER (itself, confirmed as destination)

### Header Pattern Summary

For any command traversing through a router:

| Step | Sender | Header Format | Meaning |
|------|--------|---------------|---------|
| 1 | MASTER | `!!MASTER:ARM1:...##` | MASTER → ARM1 (via ROUTER) |
| 2 | ROUTER → ARM1 | `!!MASTER:ARM1:...##` | Forward: MASTER's message destined for ARM1 |
| 3 | ARM1 | `!!ARM1:MASTER:...##` | ARM1 → MASTER (via ROUTER) |
| 4 | ROUTER → MASTER | `!!ARM1:MASTER:...##` | Forward: ARM1's message destined for MASTER |

**Key insight:** The first header is **always** the originator, and the second header is **always** the immediate recipient. The router does **not** insert itself into the header chain—it simply checks the TO field and routes accordingly.

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

Headers are preserved in order and represent the communication path:

**Two-hop example (sender → receiver via router):**
```
!!SOURCE:DESTINATION:REQUEST:COMMAND##
```

In this case:
- `headers[0]` = `SOURCE` (originator/sender)
- `headers[1]` = `DESTINATION` (intended recipient)
- `msgKind` = `REQUEST`
- `command` = `COMMAND`

The first header is **always** the originator, and the second header is **always** the intended recipient. Additional headers can represent additional routing information or context.

**Accessing headers in code:**
```cpp
String sender = cmd.getHeader(0);       // The originator
String recipient = cmd.getHeader(1);    // The intended recipient
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

(You may include this README in your project and distribute as you like — just leave a cookie for Timo 🍪.)
