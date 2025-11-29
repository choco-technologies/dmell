# cat - Concatenate and Display Files Command

## Description

The `cat` command concatenates and displays the contents of one or more files.

## Usage

```
cat <file1> [file2 ...]
```

## Arguments

- `<file1>` - Path to the first file to display (required)
- `[file2 ...]` - Optional additional files to display

## Behavior

- Files are read and displayed in sequence.
- If a file cannot be opened, an error message is printed, but processing continues with remaining files.
- Binary-safe reading, but content is displayed as text.

## Exit Codes

- `0` - Success (all files read successfully)
- `-EINVAL` - No files specified
- `-1` - At least one file could not be opened

## Examples

```bash
# Display a single file
cat /path/to/file.txt

# Display multiple files
cat file1.txt file2.txt file3.txt

# Display configuration file
cat /etc/config.conf
```
