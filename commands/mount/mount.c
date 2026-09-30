#include <dmod.h>
#include <errno.h>
#include <string.h>

/*
 * dmvfs is built into the firmware (it is not a module), its API is
 * resolved from the system like the rest of the built-in API.
 */
DMOD_BUILTIN_API( dmvfs, 1.0, bool, _mount_fs, (const char* fs_name, const char* mount_point, const char* config) );

static void print_usage(void)
{
    DMOD_LOG_ERROR("Usage: mount -t <fs> [-o <config>] [<source>] <directory>\n");
}

/**
 * @brief Entry point for the 'mount' command module.
 *
 * Mounts a file system module at a directory.
 * Usage: mount -t <fs> [-o <config>] [<source>] <directory>
 *
 * The configuration string handed to the file system is <config> when
 * given, <source> otherwise - what it means is up to the file system, e.g.
 *   mount -t dmfatfs /dev/dmsdio0/0 /mnt/sd
 *   mount -t dmffs -o flash_addr=0x08080000,flash_size=0x80000 /flash
 *
 * @param argc Number of arguments
 * @param argv Array of argument strings
 * @return int Exit code (0 on success, negative on error)
 */
int main( int argc, char** argv )
{
    const char* fs_name   = NULL;
    const char* config    = NULL;
    const char* operands[2];
    int         operand_count = 0;

    for( int i = 1; i < argc; i++ )
    {
        bool has_value = ( i + 1 < argc );
        if( strcmp(argv[i], "-t") == 0 && has_value )
        {
            fs_name = argv[++i];
        }
        else if( strcmp(argv[i], "-o") == 0 && has_value )
        {
            config = argv[++i];
        }
        else if( argv[i][0] != '-' && operand_count < 2 )
        {
            operands[operand_count++] = argv[i];
        }
        else
        {
            print_usage();
            return -EINVAL;
        }
    }

    if( fs_name == NULL || operand_count == 0 || ( operand_count == 2 && config != NULL ) )
    {
        print_usage();
        return -EINVAL;
    }

    const char* mount_point = operands[operand_count - 1];
    if( operand_count == 2 )
    {
        config = operands[0];
    }

    if( !dmvfs_mount_fs(fs_name, mount_point, config) )
    {
        DMOD_LOG_ERROR("mount: cannot mount '%s' at '%s'\n", fs_name, mount_point);
        return -EIO;
    }
    return 0;
}
