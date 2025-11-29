# tail - Display Last Lines of File Command

## Description

The `tail` command displays the last lines of a file.

## Usage

```
tail [-n <lines>] <file>
```

## Options

- `-n <lines>` - Number of lines to display (default: 10)
- `-<number>` - Alternative syntax for specifying number of lines (e.g., `-20`)

## Arguments

- `<file>` - Path to the file to display (required)

## Behavior

- By default, displays the last 10 lines of the file.
- Use `-n` option or `-<number>` syntax to specify a different number of lines.
- Reads the entire file to count lines, then displays the last N lines.

## Exit Codes

- `0` - Success
- `-EINVAL` - Invalid arguments or options
- `-1` - Failed to open file

## Examples

```bash
# Display last 10 lines (default)
tail file.txt

# Display last 20 lines
tail -n 20 file.txt

# Alternative syntax for last 20 lines
tail -20 file.txt
```

## Notes

- Unlike the Unix `tail -f` option for following file updates, this implementation only displays the current file contents without follow mode.
