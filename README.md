# dmell
DMOD Shell - A lightweight shell implementation for the DMOD module system.

## Overview

dmell is a shell interpreter designed for the DMOD (Dynamic Module) system. It provides a command-line interface for executing commands, running scripts, and managing variables.

## Features

- **Built-in Commands**: Basic shell commands like `echo`, `cd`, `pwd`, `set`, `unset`, `export`, and `exit`
- **External Command Modules**: Complex commands (`cp`, `mv`, `ls`, `cat`) available as separate DMOD modules
- **Script Execution**: Support for `.dme` script files
- **Variable Management**: Environment variables and shell variables support
- **Shebang Support**: Execute scripts with custom interpreters

## Built-in Commands

| Command | Description |
|---------|-------------|
| `echo`  | Print arguments to standard output |
| `cd`    | Change current directory |
| `pwd`   | Print current working directory |
| `set`   | Set a shell variable |
| `export`| Export a variable |
| `unset` | Unset a variable |
| `exit`  | Exit the shell with optional exit code |

## Command Modules

Complex file system commands are implemented as separate DMOD modules for better modularity:

| Module | Description | Documentation |
|--------|-------------|---------------|
| `cp`   | Copy files | [commands/cp/README.md](commands/cp/README.md) |
| `mv`   | Move/rename files | [commands/mv/README.md](commands/mv/README.md) |
| `ls`   | List directory contents | [commands/ls/README.md](commands/ls/README.md) |
| `cat`  | Display file contents | [commands/cat/README.md](commands/cat/README.md) |

## Building

### Prerequisites

- CMake 3.18 or higher
- A C compiler (GCC, Clang, etc.)
- DMOD SDK

### Build Instructions

```bash
mkdir -p build
cd build
cmake .. -DDMOD_MODE=DMOD_MODULE
cmake --build .
```

## Usage

### Running the Shell

```bash
dmell
```

### Running a Script

```bash
dmell script.dme
```

### Script Example

```bash
#!/bin/dmell

echo "Hello, dmell!"
set myvar=value
echo "myvar = $myvar"
```

## Project Structure

```
dmell/
├── src/                  # Main shell source files
├── include/              # Header files
├── commands/             # Command modules
│   ├── cp/              # Copy command module
│   ├── mv/              # Move command module
│   ├── ls/              # List command module
│   └── cat/             # Cat command module
├── examples/            # Example scripts
└── CMakeLists.txt       # Build configuration
```

## License

See [LICENSE](LICENSE) for details.
