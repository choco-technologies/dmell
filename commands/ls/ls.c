#include <dmod.h>
#include <errno.h>
#include <string.h>

/**
 * @brief Entry point for the 'ls' command module.
 * 
 * Lists directory contents.
 * Usage: ls [-a] [-l] [path]
 * 
 * @param argc Number of arguments
 * @param argv Array of argument strings
 * @return int Exit code (0 on success, negative on error)
 */
int main( int argc, char** argv )
{
    const char* path = ".";
    int show_hidden = 0;
    int long_format = 0;
    
    // Parse arguments
    for( int i = 1; i < argc; i++ )
    {
        if( argv[i][0] == '-' )
        {
            // Handle flags
            for( int j = 1; argv[i][j] != '\0'; j++ )
            {
                switch( argv[i][j] )
                {
                    case 'a':
                        show_hidden = 1;
                        break;
                    case 'l':
                        long_format = 1;
                        break;
                    default:
                        DMOD_LOG_ERROR("Unknown option: -%c\n", argv[i][j]);
                        return -EINVAL;
                }
            }
        }
        else
        {
            // Path argument
            path = argv[i];
        }
    }
    
    void* dir = Dmod_OpenDir(path);
    if( dir == NULL )
    {
        DMOD_LOG_ERROR("Failed to open directory '%s'\n", path);
        return -1;
    }
    
    const char* entry;
    while( (entry = Dmod_ReadDir(dir)) != NULL )
    {
        // Skip hidden files unless -a flag is used
        if( !show_hidden && entry[0] == '.' )
        {
            continue;
        }
        
        if( long_format )
        {
            // Simple long format - just show name (could be extended with file stats)
            Dmod_Printf("%-20s\n", entry);
        }
        else
        {
            Dmod_Printf("%s  ", entry);
        }
    }
    
    if( !long_format )
    {
        Dmod_Printf("\n");
    }
    
    Dmod_CloseDir(dir);
    return 0;
}
