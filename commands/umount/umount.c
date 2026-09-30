#include <dmod.h>
#include <dmvfs.h>
#include <errno.h>
#include <string.h>

/**
 * @brief Entry point for the 'umount' command module.
 *
 * Unmounts the file systems mounted at the given directories.
 * Usage: umount <directory1> [directory2 ...]
 *
 * @param argc Number of arguments
 * @param argv Array of argument strings
 * @return int Exit code (0 on success, negative on error)
 */
int main( int argc, char** argv )
{
    if( argc < 2 )
    {
        DMOD_LOG_ERROR("Usage: umount <directory1> [directory2 ...]\n");
        return -EINVAL;
    }

    int result = 0;

    for( int i = 1; i < argc; i++ )
    {
        if( !dmvfs_unmount_fs(argv[i]) )
        {
            DMOD_LOG_ERROR("umount: cannot unmount '%s'\n", argv[i]);
            result = -EIO;
        }
    }

    return result;
}
