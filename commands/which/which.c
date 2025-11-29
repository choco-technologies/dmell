#include <dmod.h>
#include <errno.h>
#include <string.h>

/**
 * @brief Entry point for the 'which' command module.
 * 
 * Shows the path to a DMOD module.
 * Usage: which <module_name>
 * 
 * @param argc Number of arguments
 * @param argv Array of argument strings
 * @return int Exit code (0 on success, 1 if not found, negative on error)
 */
int main( int argc, char** argv )
{
    if( argc < 2 )
    {
        DMOD_LOG_ERROR("Usage: which <module_name>\n");
        return -EINVAL;
    }

    int result = 0;
    
    for( int i = 1; i < argc; i++ )
    {
        const char* module_name = argv[i];
        char file_path[512];
        
        // Try to find the module file
        // Pass NULL for arch to use default architecture
        bool found = Dmod_FindModuleFile(module_name, NULL, file_path, sizeof(file_path));
        
        if( found )
        {
            Dmod_Printf("%s\n", file_path);
        }
        else
        {
            DMOD_LOG_ERROR("%s not found\n", module_name);
            result = 1;
        }
    }

    return result;
}
