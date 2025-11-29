#include <dmod.h>
#include <errno.h>
#include <string.h>

/**
 * @brief Entry point for the 'mkdir' command module.
 * 
 * Creates directories.
 * Usage: mkdir [-p] <directory1> [directory2 ...]
 * 
 * @param argc Number of arguments
 * @param argv Array of argument strings
 * @return int Exit code (0 on success, negative on error)
 */
int main( int argc, char** argv )
{
    if( argc < 2 )
    {
        DMOD_LOG_ERROR("Usage: mkdir [-p] <directory1> [directory2 ...]\n");
        return -EINVAL;
    }

    int create_parents = 0;
    int result = 0;
    
    for( int i = 1; i < argc; i++ )
    {
        if( argv[i][0] == '-' )
        {
            // Handle flags
            for( int j = 1; argv[i][j] != '\0'; j++ )
            {
                switch( argv[i][j] )
                {
                    case 'p':
                        create_parents = 1;
                        break;
                    default:
                        DMOD_LOG_ERROR("Unknown option: -%c\n", argv[i][j]);
                        return -EINVAL;
                }
            }
        }
        else
        {
            // Directory argument
            const char* dir_path = argv[i];
            
            if( create_parents )
            {
                // Create parent directories if needed
                size_t path_len = strlen(dir_path);
                char* path_copy = Dmod_Malloc(path_len + 1);
                if( path_copy == NULL )
                {
                    DMOD_LOG_ERROR("Memory allocation failed\n");
                    return -ENOMEM;
                }
                memcpy(path_copy, dir_path, path_len + 1);
                
                char* p = path_copy;
                // Skip leading slash
                if( *p == '/' )
                {
                    p++;
                }
                
                while( *p != '\0' )
                {
                    // Find next slash
                    while( *p != '\0' && *p != '/' )
                    {
                        p++;
                    }
                    
                    char saved = *p;
                    *p = '\0';
                    
                    // Try to create directory (ignore errors for existing directories)
                    Dmod_MakeDir(path_copy, 0755);
                    
                    *p = saved;
                    if( *p != '\0' )
                    {
                        p++;
                    }
                }
                
                Dmod_Free(path_copy);
            }
            else
            {
                // Create single directory
                int ret = Dmod_MakeDir(dir_path, 0755);
                if( ret != 0 )
                {
                    DMOD_LOG_ERROR("Failed to create directory '%s': %d\n", dir_path, ret);
                    result = -1;
                }
            }
        }
    }

    return result;
}
