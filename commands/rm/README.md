# rm - Remove Files Command

## Description

The `rm` command removes files.

## Usage

```
rm <file1> [file2 ...]
```

## Arguments

- `<file1>` - Path to the file to remove (required)
- `[file2 ...]` - Optional additional files to remove

## Behavior

- Removes specified files from the filesystem.
- If a file cannot be removed, an error is reported but processing continues.

## Exit Codes

- `0` - Success (all files removed successfully)
- `-EINVAL` - No files specified
- `-1` - Failed to remove at least one file

## Examples

```bash
# Remove a single file
rm file.txt

# Remove multiple files
rm file1.txt file2.txt file3.txt
```
