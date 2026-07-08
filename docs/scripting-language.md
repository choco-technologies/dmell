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

Export a variable to the environment (also sets it as a shell variable). When called without arguments, lists all currently set environment variables:

```bash
export PATH=/usr/bin
export    # Lists all environment variables
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

### Stream Redirection

Commands - built-in (`echo`, `module`, ...) or external DMOD modules alike -
can have their standard streams redirected to files, similar to POSIX shells:

```bash
cat file.txt > out.txt        # stdout to a file (truncated)
cat file.txt >> out.txt       # stdout to a file (appended)
grep pattern < input.txt      # stdin from a file
mycmd 2> errors.txt           # stderr to a file
mycmd 2>> errors.txt          # stderr appended to a file
mycmd &> combined.txt         # stdout and stderr to the same file
mycmd &>> combined.txt        # same, appended
mycmd > out.txt 2>&1          # stderr follows stdout into out.txt
mycmd 2>&1 > out.txt          # stderr stays on the terminal, only stdout goes to out.txt
mycmd 3> log.txt              # dmell-specific: redirect the DMOD "stdlog" stream
echo "hello" > greeting.txt   # works for built-ins too
```

Redirection operators are resolved left to right, exactly like a POSIX
shell: `2>&1` binds stderr to wherever stdout *currently* points, so its
position relative to `>out.txt` on the command line changes the result (see
the two `2>&1` examples above).

Under the hood, dmell redirects its own process's streams for the duration of
the command and restores them exactly afterward (whatever they pointed at
before, including nothing at all) - built-in commands and external modules
both write through those same streams, so one mechanism covers everything,
including redirecting an entire `.dme` script's output by putting the
redirect on the line that invokes it. Redirection requires the underlying
platform to support binding a process's streams to files; if it doesn't,
dmell reports an error rather than silently running the command unredirected.

### Background Execution

A command followed by `&` runs in the background: dmell launches it and moves
on immediately instead of waiting for it to finish.

```bash
myapp &                  # launched, prompt/next command continues right away
myapp 2>/tmp/errors.txt & # redirection still works exactly the same way
myapp & echo "next"       # "next" prints without waiting for myapp
```

Like `;`, `&` never blocks the rest of the line on the exit status of the
command it follows - the next command always runs.

Background execution is only supported for external DMOD module commands.
Built-in commands (`echo`, `cd`, `module`, ...), variable assignment, and
`.dme` scripts all execute inside dmell's own process/thread and have no
independent execution context to hand off without blocking the shell, so
`&` on one of those is rejected with an error instead of silently running
in the foreground.

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
