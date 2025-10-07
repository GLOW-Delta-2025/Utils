# CmdLib

A lightweight, cross-platform command parser and builder library for structured command strings.

## Format

CmdLib parses commands in the following format:

```
!!device:type:command:{key=val,key2=val2}##
```

**Example:**
```
!!robot:motor:move:{speed=100,direction=forward}##
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

String input = "!!robot:motor:move:{speed=100,direction=forward}##";
Command cmd;
String error;

if (parse(input, cmd, error)) {
  Serial.println(cmd.device);   // "robot"
  Serial.println(cmd.type);     // "motor"
  Serial.println(cmd.command);  // "move"
  Serial.println(cmd.getParam("speed"));     // "100"
  Serial.println(cmd.getParam("direction")); // "forward"
} else {
  Serial.println("Parse error: " + error);
}
```

### Building Commands

```cpp
Command cmd;
cmd.device = "sensor";
cmd.type = "temp";
cmd.command = "read";
cmd.setParam("unit", "celsius");
cmd.setParam("precision", "2");

String output = cmd.toString();
// Output: !!sensor:temp:read:{unit=celsius,precision=2}##
```

### Quoted Values

```cpp
!!display:lcd:print:{text="Hello World",line=1}##
```

### Flags (no value)

```cpp
!!device:system:reset:{force,verbose}##
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
| `device` | String/string | Device identifier |
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
- Proper header format (`device:type:command`)
- Balanced braces `{}`
- Proper key-value syntax

## License

*(Add your license here)*
