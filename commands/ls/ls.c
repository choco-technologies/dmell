#include <dmod.h>
#include <errno.h>
#include <string.h>

/**
 * @brief Helper function to get the filename from a given path.
 * 
 * @param path Full file path
 * @return const char* Filename extracted from the path
 */
static const char* get_filename_from_path(const char* path)
{
    const char* last_slash = strrchr(path, '/');
    if(last_slash != NULL)
    {
        return last_slash + 1;
    }
    return path;
}

/**
 * @brief Entry point for the 'ls' command module.
 * 
 * Lists directory contents or file information.
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
        // If it's not a directory, try to list it as a file
        const char* filename = get_filename_from_path(path);
        
        // Skip hidden files unless -a flag is used
        if( !show_hidden && filename[0] == '.' )
        {
            return 0;
        }
        
        // Print the filename
        if( long_format )
        {
            Dmod_Printf("%-20s\n", filename);
        }
        else
        {
            Dmod_Printf("%s\n", filename);
        }
        return 0;
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
