# DMELL Scripting Language

This document describes the syntax and features of the DMELL scripting language used in `.dme` script files.

## Overview

DMELL (DMOD Shell) is a lightweight shell interpreter designed for the DMOD (Dynamic Module) system. It provides a simple, shell-like scripting language for automation and command execution.

## File Extension

DMELL scripts use the `.dme` file extension.

## Shebang

Scripts can start with a shebang line to specify the interpreter:

```bash
#!/bin/dmell
```

## Comments

Comments start with `#` and continue to the end of the line:

```bash
# This is a comment
echo "Hello"  # Inline comment
```

## Variables

### Setting Variables

Variables can be set using the `set` command or direct assignment:

```bash
# Using set command
set myvar=value

# Direct assignment
myvar=value

# Export to environment
export PATH=/usr/bin
```

### Variable Names

Variable names must:
- Start with a letter (a-z, A-Z) or underscore (`_`)
- Contain only letters, digits (0-9), and underscores

### Referencing Variables

Variables can be referenced using the `$` prefix:

```bash
# Simple syntax
echo $myvar

# Bracket syntax (useful when followed by alphanumeric characters)
echo ${myvar}_suffix
```

### Special Variables

| Variable | Description |
|----------|-------------|
| `$?`     | Exit code of the last executed command |
| `$0`     | Name of the script or first argument |
| `$1`, `$2`, ... | Positional parameters (script arguments) |

### Unsetting Variables

Remove a variable using `unset`:

```bash
unset myvar
```

## Strings

### Double-Quoted Strings

Variables are expanded inside double-quoted strings:

```bash
set name=World
echo "Hello, $name!"  # Output: Hello, World!
```

### Single-Quoted Strings

Variables are NOT expanded inside single-quoted strings (literal):

```bash
set name=World
echo 'Hello, $name!'  # Output: Hello, $name!
```

### Escape Sequences

The following escape sequences are supported:

| Sequence | Description |
|----------|-------------|
| `\\`     | Backslash |
| `\"`     | Double quote |
| `\'`     | Single quote |
| `\n`     | Newline |
| `\r`     | Carriage return |
| `\t`     | Tab |

## Built-in Commands

### echo

Print arguments to standard output:

```bash
echo "Hello, World!"
echo $variable
echo "Multiple" "arguments"
```

### set

Set a shell variable:

```bash
set varname=value
```

### export

Export a variable to the environment (also sets it as a shell variable):

```bash
export PATH=/usr/bin
```

### unset

Remove a variable:

```bash
unset varname
unset var1 var2 var3  # Multiple variables
```

### cd

Change the current directory:

```bash
cd /path/to/directory
cd    # Without arguments, changes to HOME directory
```

### pwd

Print the current working directory:

```bash
pwd
```

### exit

Exit the script with an optional exit code:

```bash
exit      # Exit with the last command's exit code
exit 0    # Exit with success
exit 1    # Exit with error
```

## External Commands

DMELL can execute external commands and DMOD modules. If a command is not a built-in, DMELL will attempt to:

1. Execute it as a script file (if it has a shebang or `.dme` extension)
2. Run it as a DMOD module

### Running Scripts

```bash
# Run a .dme script
./myscript.dme

# Run with shebang interpreter
./script_with_shebang.sh
```

### Running DMOD Modules

External file system commands are available as DMOD modules:

```bash
# These require the corresponding modules to be installed
cp source.txt dest.txt
mv oldname.txt newname.txt
ls /path/to/directory
cat file.txt
```

## Script Example

```bash
#!/bin/dmell

# DMELL Script Example

# Set variables
set greeting=Hello
set name=World

# Print a message
echo "$greeting, $name!"

# Check exit code
echo "Exit code: $?"

# Change directory
cd /tmp
echo "Current directory:"
pwd

# Export environment variable
export MY_VAR=my_value

# Clean up
unset greeting name

# Exit successfully
exit 0
```

## Command Line Execution

DMELL scripts can be executed in several ways:

### Using the DMOD loader (on PC)

```bash
dmod_loader dmell.dmf script.dme
```

### Interactive Mode

```bash
dmod_loader dmell.dmf
```

This starts an interactive shell where you can enter commands directly.

## Maximum Line Length

The maximum length of a script line is 512 characters.

## Error Handling

Commands return exit codes:
- `0` - Success
- Non-zero - Error

The exit code of the last command is stored in the `$?` variable.

```bash
echo "Hello"
echo "Exit code was: $?"
```

## See Also

- [Main README](../README.md) - Overview of the DMELL project
- [VS Code Extension](../tools/vscode-dme/README.md) - Syntax highlighting for VS Code
