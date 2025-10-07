# CmdLib — Simple Command Builder & Parser for Arduino and C++

## 📖 Overview

`CmdLib` is a lightweight, single-header C++ library that makes it easy to **build** and **parse** structured command strings for serial communication between devices.

It’s designed for **Arduino environments** (but also works on desktop C++), and follows this format:

!![device]:[type]:[command]:{[key=value],[key=value]}##


### 🔹 Example

!!ARM1:REQUEST:SEND_STAR:{speed=3,color=red,brightness=80,size=10}##


And the confirmation could look like:

!!ARM1:CONFIRM:SEND_STAR:{speed=3,color=red,brightness=80,size=10}##


---

## ⚙️ Features

✅ Build formatted command strings easily  
✅ Parse incoming commands into structured data  
✅ Retrieve parameters by key  
✅ Works on **Arduino** and **standard C++**  
✅ Header-only — just drop in `CmdLib.h`  
✅ Arduino version uses fixed-size arrays (no `std::map` or dynamic allocation)

---

## 📦 File Structure

CmdLib/
├── CmdLib.h # The single-header library
└── examples/
└── CmdLib_SerialExample/
└── CmdLib_SerialExample.ino # Arduino demo with Serial read


---

## 🚀 Installation

### 🧩 Option 1 — Add to Arduino Project
1. Copy `CmdLib.h` into your Arduino sketch folder.
2. `#include "CmdLib.h"` in your `.ino` file.

### 🧰 Option 2 — Include in a C++ Project
```cpp
#include "CmdLib.h"

Then compile normally. No dependencies required.
🧠 Usage (Arduino Example)

#include "CmdLib.h"

String inputBuffer;

void setup() {
  Serial.begin(115200);
  Serial.println("Command Parser Ready");
}

void processCommand(const String &cmdStr) {
  cmdlib::Command cmd;
  String err;

  if (!cmdlib::parse(cmdStr, cmd, err)) {
    Serial.print("Parse error: ");
    Serial.println(err);
    return;
  }

  Serial.println("---- Command Received ----");
  Serial.print("Device: "); Serial.println(cmd.device);
  Serial.print("Type: "); Serial.println(cmd.type);
  Serial.print("Command: "); Serial.println(cmd.command);

  for (int i = 0; i < cmd.paramCount; ++i) {
    Serial.print("  ");
    Serial.print(cmd.params[i].key);
    Serial.print(" = ");
    Serial.println(cmd.params[i].value);
  }

  // Example response for REQUEST
  if (cmd.type == "REQUEST") {
    cmdlib::Command confirm;
    confirm.device = cmd.device;
    confirm.type = "CONFIRM";
    confirm.command = cmd.command;
    for (int i = 0; i < cmd.paramCount; ++i) {
      confirm.setParam(cmd.params[i].key, cmd.params[i].value);
    }
    Serial.print("Response -> ");
    Serial.println(confirm.toString());
  }
}

void loop() {
  while (Serial.available()) {
    char c = Serial.read();
    inputBuffer += c;
    if (inputBuffer.endsWith("##")) {
      processCommand(inputBuffer);
      inputBuffer = "";
    }
    if (inputBuffer.length() > 512) inputBuffer = "";
  }
}

🧩 Example Output

Send via Serial Monitor:

!!ARM1:REQUEST:SEND_STAR:{speed=3,color=red,brightness=80,size=10}##

Arduino Output:

Command Parser Ready
---- Command Received ----
Device: ARM1
Type: REQUEST
Command: SEND_STAR
  speed = 3
  color = red
  brightness = 80
  size = 10
Response -> !!ARM1:CONFIRM:SEND_STAR:{speed=3,color=red,brightness=80,size=10}##
---------------------------

⚙️ Advanced
Building Commands Manually

cmdlib::Command cmd;
cmd.device = "ARM1";
cmd.type = "REQUEST";
cmd.command = "MOVE";
cmd.setParam("speed", "5");
cmd.setParam("angle", "90");

Serial.println(cmd.toString());
// -> !!ARM1:REQUEST:MOVE:{speed=5,angle=90}##

Getting Parameters

String speed = cmd.getParam("speed", "0");

🔧 Configuration
Option	Default	Description
CMDLIB_MAX_PARAMS	12	Max number of parameters stored in Arduino mode
CMDLIB_ARDUINO	(auto)	Force Arduino mode manually if needed

Example:

#define CMDLIB_MAX_PARAMS 20
#include "CmdLib.h"

🧱 Supported Platforms

    ✅ Arduino (UNO, Mega, ESP32, RP2040, etc.)

    ✅ PlatformIO

    ✅ Desktop C++ (Linux, macOS, Windows)

🧰 License

MIT License — free to use, modify, and distribute.
💬 Example Protocol Flow
Step	Sender	Message
1	Controller	!!ARM1:REQUEST:SEND_STAR:{speed=3,color=red}##
2	Device	!!ARM1:CONFIRM:SEND_STAR:{speed=3,color=red}##
3	Controller	Ready for next command
🌟 Roadmap

Command dispatcher (onCommand("SEND_STAR", handler))

Typed getters (getInt(), getFloat(), etc.)

Lightweight CRC or checksum field support

    Stream parsing for long serial messages

💡 Tip

You can easily extend this library to trigger functions when a certain command arrives, for example:

if (cmd.command == "MOVE_ARM") moveArm(cmd.getParam("speed"));
