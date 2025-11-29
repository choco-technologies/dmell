# mv - Move/Rename Command

## Description

The `mv` command moves or renames files from a source location to a destination location.

## Usage

```
mv <source> <destination>
```

## Arguments

- `<source>` - Path to the source file to move/rename
- `<destination>` - Path to the destination file or directory

## Behavior

- If the destination is a directory, the source file will be moved into that directory with its original filename.
- If the destination is a file path, the source file will be renamed/moved to that exact path.
- Uses the underlying `Dmod_Rename` function for atomic move operations.

## Exit Codes

- `0` - Success
- `-EINVAL` - Invalid arguments provided
- `-ENOMEM` - Memory allocation failed
- Non-zero - Failed to move/rename the file

## Examples

```bash
# Rename a file
mv /path/to/oldname.txt /path/to/newname.txt

# Move a file into a directory
mv /path/to/file.txt /path/to/directory/
```
