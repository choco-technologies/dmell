# DMELL Script Language Support

Syntax highlighting for DMELL Script (`.dme`) files in Visual Studio Code.

## Features

This extension provides syntax highlighting for DMELL script files, including:

- **Shebang**: Lines starting with `#!` (e.g., `#!/bin/dmell`)
- **Comments**: Lines starting with `#`
- **Built-in Commands**: `echo`, `set`, `unset`, `export`, `cd`, `pwd`, `exit`
- **Variables**: 
  - Simple variables: `$varname`
  - Bracket variables: `${varname}`
  - Special variables: `$?` (last exit code), `$0`, `$1`, etc. (positional parameters)
- **Strings**: Double-quoted and single-quoted strings
- **Variable Expansion**: Variables expanded within double-quoted strings
- **Escape Sequences**: Support for `\\`, `\"`, `\'`, `\n`, `\r`, `\t`

## Script Format

DMELL script files (`.dme`) use a shell-like syntax:

```bash
#!/bin/dmell

# This is a comment
echo "Hello, dmell!"

# Set a variable
set myvar=value
export PATH=/usr/bin

# Variable assignment (alternative syntax)
myvar=value

# Use variables
echo "myvar = $myvar"
echo "Exit code: $?"

# Change directory
cd /home/user
pwd

# Exit with code
exit 0
```

### Built-in Commands

| Command | Description | Example |
|---------|-------------|---------|
| `echo`  | Print arguments to standard output | `echo "Hello World"` |
| `set`   | Set a shell variable | `set myvar=value` |
| `export`| Export a variable to environment | `export PATH=/usr/bin` |
| `unset` | Unset a variable | `unset myvar` |
| `cd`    | Change current directory | `cd /home/user` |
| `pwd`   | Print current working directory | `pwd` |
| `exit`  | Exit the shell with optional exit code | `exit 0` |

### Variables

Variables can be referenced in two ways:

- **Simple syntax**: `$varname`
- **Bracket syntax**: `${varname}` (useful when variable is followed by alphanumeric characters)

Special variables:
- `$?` - Exit code of the last executed command
- `$0`, `$1`, `$2`, ... - Positional parameters (script arguments)

### Strings

- **Double-quoted strings** (`"..."`) - Variables are expanded
- **Single-quoted strings** (`'...'`) - Variables are NOT expanded (literal)

## Installation

### From Source

1. Copy the `vscode-dme` directory to your VS Code extensions folder:
   - **Windows**: `%USERPROFILE%\.vscode\extensions\`
   - **macOS/Linux**: `~/.vscode/extensions/`

2. Reload VS Code

### Package and Install

1. Install `vsce` (Visual Studio Code Extension Manager):
   ```bash
   npm install -g @vscode/vsce
   ```

2. Package the extension:
   ```bash
   cd tools/vscode-dme
   vsce package
   ```

3. Install the generated `.vsix` file:
   ```bash
   code --install-extension dmell-script-1.0.0.vsix
   ```

## Usage

Once installed, the extension automatically provides syntax highlighting for all files with the `.dme` extension.

## License

This extension is part of the DMELL project. See the main repository for license information.

## Repository

https://github.com/choco-technologies/dmell
