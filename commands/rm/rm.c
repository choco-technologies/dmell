#include <dmod.h>
#include <errno.h>
#include <string.h>

/**
 * @brief Entry point for the 'rm' command module.
 * 
 * Removes files.
 * Usage: rm <file1> [file2 ...]
 * 
 * @param argc Number of arguments
 * @param argv Array of argument strings
 * @return int Exit code (0 on success, negative on error)
 */
int main( int argc, char** argv )
{
    if( argc < 2 )
    {
        DMOD_LOG_ERROR("Usage: rm <file1> [file2 ...]\n");
        return -EINVAL;
    }

    int result = 0;
    
    for( int i = 1; i < argc; i++ )
    {
        const char* file_path = argv[i];
        
        int ret = Dmod_FileRemove(file_path);
        if( ret != 0 )
        {
            DMOD_LOG_ERROR("Failed to remove file '%s': %d\n", file_path, ret);
            result = -1;
        }
    }

    return result;
}
