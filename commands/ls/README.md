# ls - List Directory Contents Command

## Description

The `ls` command lists the contents of a directory or displays information about a file.

## Usage

```
ls [-a] [-l] [path]
```

## Options

- `-a` - Show all files, including hidden files (files starting with `.`)
- `-l` - Use long listing format (one entry per line)

## Arguments

- `[path]` - Optional path to a directory or file to list. Defaults to current directory (`.`)

## Behavior

- When given a directory path, lists the contents of the directory.
- When given a file path, displays the filename.
- By default, hidden files (starting with `.`) are not shown.
- By default, entries are displayed in a compact format separated by spaces.
- The `-l` flag shows one entry per line in a wider format.

## Exit Codes

- `0` - Success
- `-EINVAL` - Invalid option provided
- `-1` - Path does not exist or cannot be accessed

## Examples

```bash
# List current directory
ls

# List specific directory
ls /path/to/directory

# List a specific file
ls /path/to/file

# List all files including hidden
ls -a

# Long format listing
ls -l

# Combined options
ls -la /path/to/directory
```
