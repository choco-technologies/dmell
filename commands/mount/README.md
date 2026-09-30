# mount - Mount File System Command

## Description

The `mount` command mounts a file system module (e.g. `dmfatfs`, `dmffs`,
`dmramfs`) at a directory through dmvfs.

## Usage

```
mount -t <fs> [-o <config>] [<source>] <directory>
```

## Arguments

- `-t <fs>` - Name of the file system module (required)
- `-o <config>` - Configuration string handed to the file system
- `<source>` - Handed to the file system as its configuration string when
  `-o` is not given (typically a device path)
- `<directory>` - Mount point (required)

What the configuration string means is up to the file system - see its
documentation. `<source>` and `-o` cannot be used together.

## Behavior

- Loads the file system module if it is not loaded yet.
- Calls `dmvfs_mount_fs(<fs>, <directory>, <config>)`.

## Exit Codes

- `0` - Success
- `-EINVAL` - Invalid arguments
- `-EIO` - The file system could not be mounted

## Examples

```bash
# FAT file system of an SD card
mkdir /mnt
mkdir /mnt/sd
mount -t dmfatfs /dev/dmsdio0/0 /mnt/sd

# Flash file system with an explicit configuration
mount -t dmffs -o flash_addr=0x08080000,flash_size=0x80000 /flash

# RAM file system (no configuration)
mount -t dmramfs /tmp
```
