# umount - Unmount File System Command

## Description

The `umount` command unmounts file systems mounted with `mount`.

## Usage

```
umount <directory1> [directory2 ...]
```

## Arguments

- `<directory1>` - Mount point to unmount (required)
- `[directory2 ...]` - Optional additional mount points

## Behavior

- Calls `dmvfs_unmount_fs()` for every mount point given.
- If a mount point cannot be unmounted, an error is reported but processing
  continues.

## Exit Codes

- `0` - Success (all mount points unmounted)
- `-EINVAL` - No mount point specified
- `-EIO` - Failed to unmount at least one mount point

## Examples

```bash
umount /mnt/sd
```
