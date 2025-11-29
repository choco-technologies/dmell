# cp - Copy Command

## Description

The `cp` command copies files from a source location to a destination location.

## Usage

```
cp <source> <destination>
```

## Arguments

- `<source>` - Path to the source file to copy
- `<destination>` - Path to the destination file or directory

## Behavior

- If the destination is a directory, the source file will be copied into that directory with its original filename.
- If the destination is a file path, the source file will be copied to that exact path.
- Binary-safe copy operation that preserves file contents exactly.

## Exit Codes

- `0` - Success
- `-EINVAL` - Invalid arguments provided
- `-ENOMEM` - Memory allocation failed
- `-1` - Failed to open source or destination file

## Examples

```bash
# Copy a file to another location
cp /path/to/source.txt /path/to/destination.txt

# Copy a file into a directory
cp /path/to/source.txt /path/to/directory/
```
