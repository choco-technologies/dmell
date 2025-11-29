# grep - Search Text in Files Command

## Description

The `grep` command searches for patterns in files and displays matching lines.

## Usage

```
grep [-i] [-n] [-v] <pattern> <file1> [file2 ...]
```

## Options

- `-i` - Ignore case when matching
- `-n` - Show line numbers for matching lines
- `-v` - Invert match (show lines that don't match the pattern)

## Arguments

- `<pattern>` - Text pattern to search for (required)
- `<file1>` - Path to the first file to search (required)
- `[file2 ...]` - Optional additional files to search

## Behavior

- Searches for exact text pattern matches (not regular expressions).
- When searching multiple files, the filename is shown before each match.
- Each matching line is printed to standard output.

## Exit Codes

- `0` - At least one match was found
- `1` - No matches found
- `-EINVAL` - Invalid arguments or options

## Examples

```bash
# Search for "error" in a file
grep error log.txt

# Case-insensitive search
grep -i ERROR log.txt

# Show line numbers
grep -n error log.txt

# Invert match (show lines without "error")
grep -v error log.txt

# Search multiple files
grep error file1.txt file2.txt

# Combine options
grep -in error log.txt
```

## Notes

- This implementation supports simple text pattern matching only.
- Regular expressions are not supported.
