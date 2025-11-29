# Change Log

All notable changes to the "DMELL Script Language Support" extension will be documented in this file.

## [1.0.0] - 2024-11-29

### Added
- Initial release
- Syntax highlighting for `.dme` (DMELL Script) files
- Support for shebang lines (`#!/bin/dmell`)
- Support for comments (lines starting with `#`)
- Highlighting for built-in commands: `echo`, `set`, `unset`, `export`, `cd`, `pwd`, `exit`
- Variable highlighting:
  - Simple variables: `$varname`
  - Bracket variables: `${varname}`
  - Special variables: `$?` (exit code), `$0`, `$1`, etc. (positional parameters)
- String highlighting for double and single quoted strings
- Variable expansion within double-quoted strings
- Escape sequence highlighting
- Language configuration for auto-closing pairs and bracket matching
