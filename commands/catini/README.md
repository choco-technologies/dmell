# catini - Display INI Files with Syntax Highlighting

## Description

The `catini` command displays the contents of INI files with syntax highlighting using VT100 escape codes. It works similarly to `cat` but colorizes different elements of INI file syntax.

## Usage

```
catini <file1> [file2 ...]
```

## Arguments

- `<file1>` - Path to the first INI file to display (required)
- `[file2 ...]` - Optional additional INI files to display

## Syntax Highlighting

The command uses VT100 color codes to highlight different INI syntax elements:

- **Sections** (`[section]`) - Displayed in bright cyan
- **Keys** (`key=`) - Displayed in bright yellow
- **Values** (after `=`) - Displayed in green
- **Comments** (lines starting with `;` or `#`) - Displayed in dark gray

## Behavior

- Files are read and displayed in sequence with syntax highlighting
- If a file cannot be opened, an error message is printed, but processing continues with remaining files
- The highlighting recognizes:
  - Section headers: `[section_name]`
  - Key-value pairs: `key = value`
  - Comments: Lines starting with `;` or `#` (with optional leading whitespace)
  - Empty lines are displayed as-is

## Exit Codes

- `0` - Success (all files read successfully)
- `-EINVAL` - No files specified
- `-1` - At least one file could not be opened

## Examples

```bash
# Display a single INI file
catini /path/to/config.ini

# Display multiple INI files
catini app.ini database.ini settings.ini

# Display system configuration
catini /etc/app/config.ini
```

## Sample Output

Given an INI file like:
```ini
; This is a comment
[Database]
host = localhost
port = 5432

[Application]
name = MyApp
debug = true
```

The command will display each element in different colors:
- Comments in dark gray
- `[Database]` and `[Application]` in bright cyan
- `host`, `port`, `name`, `debug` in bright yellow
- `localhost`, `5432`, `MyApp`, `true` in green
