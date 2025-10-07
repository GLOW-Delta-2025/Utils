# CmdLib

A lightweight, cross-platform command parser and builder library for structured command strings.

## Format

CmdLib parses commands in the following format:

```
!!type:command:{key=val,key2=val2}##
```

**Example:**
```
!!REQUEST:SEND_STAR:{speed=3,color=red,brightness=80,size=10}##
```

## Features

- **Cross-platform**: Works on both Arduino and standard C++ (STL)
- **Automatic detection**: Automatically uses Arduino mode when `ARDUINO` is defined
- **Simple API**: Easy-to-use command building and parsing
- **Flexible parameters**: Supports key-value pairs and flags
- **Quoted values**: Parse values with spaces using quotes
- **Small footprint**: Arduino version uses fixed arrays (no dynamic allocation)

## Usage

### Basic Parsing

```cpp
#include "CmdLib.h"

using namespace cmdlib;

String input = "!!REQUEST:SEND_STAR:{speed=3,color=red,brightness=80,size=10}##";
Command cmd;
String error;

if (parse(input, cmd, error)) {
  Serial.println(cmd.type);     // "REQUEST"
  Serial.println(cmd.command);  // "SEND_STAR"
  Serial.println(cmd.getParam("speed"));     // "3"
  Serial.println(cmd.getParam("color")); // "red"
} else {
  Serial.println("Parse error: " + error);
}
```

### Building Commands

```cpp
Command cmd;
cmd.type = "CONFIRM";
cmd.command = "SEND_STAR";
cmd.setParam("speed", "3");
cmd.setParam("color", "red");
cmd.setParam("brightness", "80");
cmd.setParam("size", "10");


String output = cmd.toString();
// Output: !!CONFIRM:SEND_STAR:{speed=3,color=red,brightness=80,size=10}##
```

## Configuration

### Arduino Mode

Arduino mode is automatically enabled when `ARDUINO` is defined. You can also force it:

```cpp
#define CMDLIB_ARDUINO
#include "CmdLib.h"
```

### Maximum Parameters (Arduino only)

The default maximum is 12 parameters. To change:

```cpp
#define CMDLIB_MAX_PARAMS 20
#include "CmdLib.h"
```

## API Reference

### Command Structure

| Field | Type | Description |
|-------|------|-------------|
| `type` | String/string | Command type |
| `command` | String/string | Command name |
| `params` | Array/map | Key-value parameters |

### Methods

- **`setParam(key, value)`** - Set or update a parameter
- **`getParam(key, default="")`** - Get parameter value (returns default if not found)
- **`toString()`** - Convert command to string format
- **`clear()`** - Reset command to empty state

### Functions

- **`bool parse(input, cmd, error)`** - Parse command string
  - Returns `true` on success
  - Returns `false` on failure (error message in `error` parameter)

## Error Handling

The parser validates:
- Prefix `!!` and suffix `##`
- Proper header format (`type:command`)
- Balanced braces `{}`
- Proper key-value syntax

## License

Timo's Cookie License usage cost 1 cookie
