# ls - List Directory Contents Command

## Description

The `ls` command lists the contents of a directory.

## Usage

```
ls [-a] [-l] [path]
```

## Options

- `-a` - Show all files, including hidden files (files starting with `.`)
- `-l` - Use long listing format (one entry per line)

## Arguments

- `[path]` - Optional path to the directory to list. Defaults to current directory (`.`)

## Behavior

- By default, hidden files (starting with `.`) are not shown.
- By default, entries are displayed in a compact format separated by spaces.
- The `-l` flag shows one entry per line in a wider format.

## Exit Codes

- `0` - Success
- `-EINVAL` - Invalid option provided
- `-1` - Failed to open directory

## Examples

```bash
# List current directory
ls

# List specific directory
ls /path/to/directory

# List all files including hidden
ls -a

# Long format listing
ls -l

# Combined options
ls -la /path/to/directory
```
