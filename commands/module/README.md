# module - DMOD Module Management Command

## Description

The `module` command provides management capabilities for DMOD modules, allowing users to load, unload, enable, disable, and inspect modules in the system.

## Usage

```
module <command> [arguments]
```

## Commands

### load

Load a module by name.

```bash
module load <name>
```

**Arguments:**
- `<name>` - Name of the module to load (required)

**Examples:**
```bash
# Load the ls module
module load ls

# Load the cat module
module load cat
```

### unload

Unload a previously loaded module.

```bash
module unload <name>
```

**Arguments:**
- `<name>` - Name of the module to unload (required)

**Examples:**
```bash
# Unload the ls module
module unload ls
```

### enable

Enable a module (loads and initializes it).

```bash
module enable <name>
```

**Arguments:**
- `<name>` - Name of the module to enable (required)

**Examples:**
```bash
# Enable the grep module
module enable grep
```

### disable

Disable a module (stops and unloads it).

```bash
module disable <name>
```

**Arguments:**
- `<name>` - Name of the module to disable (required)

**Examples:**
```bash
# Disable the grep module
module disable grep
```

### info

Display detailed information about a module.

```bash
module info <name>
```

**Arguments:**
- `<name>` - Name of the module to inspect (required)

**Output includes:**
- Module name
- Version
- Author
- Type (Application/Library)
- File location
- File size
- Current state (loaded, enabled, used)
- Required modules

**Examples:**
```bash
# Show information about the ls module
module info ls
```

### list

List all available modules with their current state.

```bash
module list
```

**Output columns:**
- Name - Module name
- Loaded - Whether the module is loaded (Yes/No)
- Enabled - Whether the module is enabled (Yes/No)
- Used - Whether the module is currently in use (Yes/No)

**Examples:**
```bash
# List all modules
module list
```

**Note:** The list command currently checks a predefined set of common modules (dmell, cp, mv, ls, cat, mkdir, touch, head, tail, grep, rm, rmdir, find, which, printf, module). Only modules that exist in the DMOD repository will be displayed.

## Exit Codes

- `0` - Command executed successfully
- `-EINVAL` - Invalid arguments or usage
- `-1` - Operation failed (e.g., module not found, load/unload failed)

## Notes

- Modules must be available in the DMOD module repository to be loaded
- Some modules may have dependencies (required modules) that must be loaded first
- Attempting to unload a module that is in use will fail
- Module files have the `.dmf` extension and are located in the DMOD repository

## See Also

- `which` - Locate the path to a module file
