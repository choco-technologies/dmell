# find - Search for Files Command

## Description

The `find` command searches for files matching a pattern in a directory tree.

## Usage

```
find [path] -name <pattern>
```

## Options

- `-name <pattern>` - Pattern to match filenames against (required)

## Arguments

- `[path]` - Directory to start searching from (default: current directory `.`)

## Pattern Wildcards

- `*` - Matches any sequence of characters
- `?` - Matches any single character

## Behavior

- Recursively searches through directories starting from the specified path.
- Prints the full path of each matching file.
- Skips `.` and `..` directory entries.

## Exit Codes

- `0` - Success
- `-EINVAL` - Invalid arguments or missing -name option

## Examples

```bash
# Find all .c files in current directory
find . -name "*.c"

# Find all files named "config" in /etc
find /etc -name "config"

# Find files matching pattern
find . -name "test_*.txt"
```
