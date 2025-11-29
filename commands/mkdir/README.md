# mkdir - Create Directory Command

## Description

The `mkdir` command creates new directories.

## Usage

```
mkdir [-p] <directory1> [directory2 ...]
```

## Options

- `-p` - Create parent directories as needed (no error if existing)

## Arguments

- `<directory1>` - Path to the directory to create (required)
- `[directory2 ...]` - Optional additional directories to create

## Behavior

- By default, creates a single directory and fails if parent directories don't exist.
- The `-p` flag creates all necessary parent directories.
- Uses permission mode 0755 for created directories.

## Exit Codes

- `0` - Success (all directories created successfully)
- `-EINVAL` - Invalid arguments or unknown option
- `-ENOMEM` - Memory allocation failed
- `-1` - Failed to create at least one directory

## Examples

```bash
# Create a single directory
mkdir data

# Create nested directories with -p flag
mkdir -p /path/to/nested/directory

# Create multiple directories
mkdir dir1 dir2 dir3
```
