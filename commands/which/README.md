# which - Locate Module Command

## Description

The `which` command shows the path to a DMOD module file.

## Usage

```
which <module_name> [module_name2 ...]
```

## Arguments

- `<module_name>` - Name of the module to locate (required)
- `[module_name2 ...]` - Optional additional modules to locate

## Behavior

- Searches for the specified DMOD module file using `Dmod_FindModuleFile`.
- Prints the full path to the module if found.
- Reports an error if the module is not found.

## Exit Codes

- `0` - All modules found successfully
- `1` - At least one module was not found
- `-EINVAL` - No module name specified

## Examples

```bash
# Find the path to the ls module
which ls

# Find paths to multiple modules
which ls cat cp
```
