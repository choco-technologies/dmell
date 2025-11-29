# head - Display First Lines of File Command

## Description

The `head` command displays the first lines of a file.

## Usage

```
head [-n <lines>] <file>
```

## Options

- `-n <lines>` - Number of lines to display (default: 10)
- `-<number>` - Alternative syntax for specifying number of lines (e.g., `-20`)

## Arguments

- `<file>` - Path to the file to display (required)

## Behavior

- By default, displays the first 10 lines of the file.
- Use `-n` option or `-<number>` syntax to specify a different number of lines.

## Exit Codes

- `0` - Success
- `-EINVAL` - Invalid arguments or options
- `-1` - Failed to open file

## Examples

```bash
# Display first 10 lines (default)
head file.txt

# Display first 20 lines
head -n 20 file.txt

# Alternative syntax for first 20 lines
head -20 file.txt
```
