# rmdir - Remove Empty Directory Command

## Description

The `rmdir` command removes empty directories.

## Usage

```
rmdir <directory1> [directory2 ...]
```

## Arguments

- `<directory1>` - Path to the directory to remove (required)
- `[directory2 ...]` - Optional additional directories to remove

## Behavior

- Removes specified empty directories from the filesystem.
- The directory must be empty for removal to succeed.
- If a directory cannot be removed, an error is reported but processing continues.

## Exit Codes

- `0` - Success (all directories removed successfully)
- `-EINVAL` - No directories specified
- `-1` - Failed to remove at least one directory

## Examples

```bash
# Remove a single empty directory
rmdir empty_dir

# Remove multiple empty directories
rmdir dir1 dir2 dir3
```
