#include <dmod.h>
#include <errno.h>
#include <string.h>

/**
 * @brief Entry point for the 'rmdir' command module.
 * 
 * Removes empty directories.
 * Usage: rmdir <directory1> [directory2 ...]
 * 
 * @param argc Number of arguments
 * @param argv Array of argument strings
 * @return int Exit code (0 on success, negative on error)
 */
int main( int argc, char** argv )
{
    if( argc < 2 )
    {
        DMOD_LOG_ERROR("Usage: rmdir <directory1> [directory2 ...]\n");
        return -EINVAL;
    }

    int result = 0;
    
    for( int i = 1; i < argc; i++ )
    {
        const char* dir_path = argv[i];
        
        int ret = Dmod_RemoveDir(dir_path);
        if( ret != 0 )
        {
            DMOD_LOG_ERROR("Failed to remove directory '%s': %d\n", dir_path, ret);
            result = -1;
        }
    }

    return result;
}
