# printf - Format and Print Command

## Description

The `printf` command formats and prints text with support for format specifiers and escape sequences.

## Usage

```
printf <format> [arguments...]
```

## Arguments

- `<format>` - Format string with optional format specifiers (required)
- `[arguments...]` - Arguments to substitute into format string

## Format Specifiers

- `%s` - String
- `%d`, `%i` - Integer (decimal)
- `%x` - Integer (hexadecimal lowercase)
- `%X` - Integer (hexadecimal uppercase)
- `%c` - Character
- `%%` - Literal percent sign

## Escape Sequences

- `\n` - Newline
- `\t` - Tab
- `\r` - Carriage return
- `\\` - Backslash
- `\0` - End output

## Exit Codes

- `0` - Success
- `-EINVAL` - No format string specified

## Examples

```bash
# Print a simple string
printf "Hello, World!\n"

# Print with string substitution
printf "Name: %s\n" "John"

# Print with integer
printf "Count: %d\n" 42

# Print multiple arguments
printf "%s is %d years old\n" "Alice" 25

# Print hex value
printf "Hex: 0x%x\n" 255
```
