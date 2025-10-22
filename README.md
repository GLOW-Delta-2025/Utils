# Command & Keep-Alive Library Suite

A collection of lightweight, cross-platform C++ libraries for Arduino and embedded systems that provide structured command parsing, building, and keep-alive connectivity monitoring.

---

## Repository Structure

```
.
├── CommandLibary/          # Command parsing & building
│   ├── CmdLib.h           # Core library (header-only)
│   ├── CmdLibTest.cpp     # Unit tests
│   └── README.md          # CmdLib documentation
│
├── KeepAlive/             # Keep-alive / timeout monitoring
│   ├── PingPong.h         # Handler definition
│   ├── PingPong.cpp       # Implementation
│   └── README.md          # PingPong documentation
│
└── README.md              # This file
```

---

## Modules Overview

### 1. **CmdLib** — Structured Command Protocol

A lightweight, header-only library that provides a standardized format for building and parsing commands with named parameters. Supports both Arduino and standard C++.

**Key Features:**
- Cross-platform: Arduino (`String`) and C++ (`std::string`)
- Named parameters only: `key=value` pairs (no positional arguments)
- Multi-hop headers for routing: `HEADER1:HEADER2:...`
- Compact binary-friendly format: `!!HEADER:MSG_KIND:COMMAND{key=val}##`
- Small footprint: Arduino builds use fixed arrays, no dynamic allocation
- Full round-trip serialization support

**Quick Example:**
```cpp
cmdlib::Command cmd;
cmd.msgKind = "REQUEST";
cmd.command = "MAKE_STAR";
cmd.setNamed("speed", "100");
cmd.setNamed("color", "red");
String output = cmd.toString();
// Output: !!REQUEST:MAKE_STAR{speed=100,color=red}##
```

**Documentation:** See [CommandLibary/README.md](CommandLibary/README.md)

---

### 2. **PingPongHandler** — Keep-Alive & Timeout Detection

An Arduino library that implements a bidirectional PING/PONG keep-alive protocol to detect connection idle states and timeouts. Built on top of CmdLib for reliable command handling.

**Key Features:**
- Automatic timeout detection with configurable idle window
- Global `PING_IDLE` flag accessible from anywhere
- Bidirectional: responds to PINGs and can send PINGs to other devices
- Serial port abstraction (works with Serial, Serial1, SoftwareSerial, etc.)
- Integrated with CmdLib for structured communication

**Quick Example:**
```cpp
void setup() {
  Serial.begin(115200);
  PingPong.init(30000);  // 30-second idle timeout
}

void loop() {
  PingPong.update();  // Check for timeouts
  
  if (Serial.available()) {
    String cmd = Serial.readStringUntil('\n');
    PingPong.processRawCommand(cmd);
  }
  
  if (PingPong.isIdle()) {
    Serial.println("Connection lost");
  }
}
```

**Documentation:** See [KeepAlive/README.md](KeepAlive/README.md)
