# dmell
DMOD Shell - A lightweight shell implementation for the DMOD module system.

## Overview

dmell is a shell interpreter designed for the DMOD (Dynamic Module) system. It provides a command-line interface for executing commands, running scripts, and managing variables.

## Features

- **Built-in Commands**: Basic shell commands like `echo`, `cd`, `pwd`, `set`, `unset`, `export`, and `exit`
- **External Command Modules**: Complex commands (`cp`, `mv`, `ls`, `cat`, `mkdir`, `touch`, `head`, `tail`, `grep`, `rm`, `rmdir`, `find`, `which`, `printf`) available as separate DMOD modules
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
| `module`| Manage DMOD modules (load, unload, enable, disable, info, list) |

### Module Command

The `module` command provides module management functionality:

- `module load <name>` - Load a module by name
- `module unload <name>` - Unload a loaded module
- `module enable <name>` - Enable a module
- `module disable <name>` - Disable a module
- `module info <name>` - Display detailed information about a module (name, version, author, path, architecture, required modules)
- `module list` - List all available modules with their names, versions, and paths

Example usage:
```bash
# List all available modules
module list

# Get information about a specific module
module info dmell

# Load a module
module load my_module

# Enable a module
module enable my_module

# Disable a module
module disable my_module

# Unload a module
module unload my_module
```

## Command Modules

Complex file system commands are implemented as separate DMOD modules for better modularity.

These modules can be installed separately using the `dmf-get` package manager tool:

```bash
dmf-get cp
dmf-get mv
dmf-get ls
dmf-get cat
dmf-get catini
dmf-get mkdir
dmf-get touch
dmf-get head
dmf-get tail
dmf-get grep
dmf-get rm
dmf-get rmdir
dmf-get find
dmf-get which
dmf-get printf
```

| Module | Description | Documentation |
|--------|-------------|---------------|
| `cp`   | Copy files | [commands/cp/README.md](commands/cp/README.md) |
| `mv`   | Move/rename files | [commands/mv/README.md](commands/mv/README.md) |
| `ls`   | List directory contents | [commands/ls/README.md](commands/ls/README.md) |
| `cat`  | Display file contents | [commands/cat/README.md](commands/cat/README.md) |
| `catini`| Display INI files with syntax highlighting | [commands/catini/README.md](commands/catini/README.md) |
| `mkdir`| Create directories | [commands/mkdir/README.md](commands/mkdir/README.md) |
| `touch`| Create empty files / update mtime | [commands/touch/README.md](commands/touch/README.md) |
| `head` | Display first lines of file | [commands/head/README.md](commands/head/README.md) |
| `tail` | Display last lines of file | [commands/tail/README.md](commands/tail/README.md) |
| `grep` | Search text in files | [commands/grep/README.md](commands/grep/README.md) |
| `rm`   | Remove files | [commands/rm/README.md](commands/rm/README.md) |
| `rmdir`| Remove empty directories | [commands/rmdir/README.md](commands/rmdir/README.md) |
| `find` | Search for files | [commands/find/README.md](commands/find/README.md) |
| `which`| Locate module path | [commands/which/README.md](commands/which/README.md) |
| `printf`| Format and print text | [commands/printf/README.md](commands/printf/README.md) |

## Building

### Prerequisites

- CMake 3.18 or higher
- A C compiler (GCC, Clang, etc.)
- DMOD SDK

### Build Instructions

```bash
mkdir -p build
cd build
cmake ..
cmake --build .
```

## Usage

### Running the Shell on PC

On PC, dmell modules are executed using the `dmod_loader` tool from the DMOD repository:

```bash
dmod_loader dmell.dmf
```

For more details about the loader, see the [dmod repository](https://github.com/choco-technologies/dmod).

### Running a Script

```bash
dmod_loader dmell.dmf script.dme
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
│   ├── cat/             # Cat command module
│   ├── catini/          # INI file viewer with syntax highlighting module
│   ├── mkdir/           # Create directory module
│   ├── touch/           # Create empty file module
│   ├── head/            # Display first lines module
│   ├── tail/            # Display last lines module
│   ├── grep/            # Search text module
│   ├── rm/              # Remove files module
│   ├── rmdir/           # Remove directories module
│   ├── find/            # Search for files module
│   ├── which/           # Locate module module
│   └── printf/          # Format and print module
├── examples/            # Example scripts
└── CMakeLists.txt       # Build configuration
```

## License

See [LICENSE](LICENSE) for details.
