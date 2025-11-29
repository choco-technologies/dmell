#include <dmod.h>
#include <errno.h>
#include <string.h>

/**
 * @brief Entry point for the 'touch' command module.
 * 
 * Creates empty files or updates modification time.
 * Usage: touch <file1> [file2 ...]
 * 
 * @param argc Number of arguments
 * @param argv Array of argument strings
 * @return int Exit code (0 on success, negative on error)
 */
int main( int argc, char** argv )
{
    if( argc < 2 )
    {
        DMOD_LOG_ERROR("Usage: touch <file1> [file2 ...]\n");
        return -EINVAL;
    }

    int result = 0;
    
    for( int i = 1; i < argc; i++ )
    {
        const char* file_path = argv[i];
        
        // Check if file exists
        if( Dmod_FileAvailable(file_path) )
        {
            // File exists - open and close to update access time (best effort)
            void* file = Dmod_FileOpen(file_path, "r+");
            if( file != NULL )
            {
                Dmod_FileClose(file);
            }
        }
        else
        {
            // File doesn't exist - create an empty file
            void* file = Dmod_FileOpen(file_path, "w");
            if( file == NULL )
            {
                DMOD_LOG_ERROR("Failed to create file '%s'\n", file_path);
                result = -1;
            }
            else
            {
                Dmod_FileClose(file);
            }
        }
    }

    return result;
}
