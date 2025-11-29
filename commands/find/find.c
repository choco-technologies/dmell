#include <dmod.h>
#include <errno.h>
#include <string.h>

/**
 * @brief Simple pattern matching with wildcards.
 * 
 * @param pattern Pattern to match (supports * and ? wildcards)
 * @param text Text to match against
 * @return int 1 if matches, 0 otherwise
 */
static int match_pattern(const char* pattern, const char* text)
{
    while( *pattern != '\0' && *text != '\0' )
    {
        if( *pattern == '*' )
        {
            // Skip consecutive asterisks
            while( *pattern == '*' )
            {
                pattern++;
            }
            
            if( *pattern == '\0' )
            {
                return 1;  // Trailing * matches everything
            }
            
            // Try to match the rest
            while( *text != '\0' )
            {
                if( match_pattern(pattern, text) )
                {
                    return 1;
                }
                text++;
            }
            return 0;
        }
        else if( *pattern == '?' || *pattern == *text )
        {
            pattern++;
            text++;
        }
        else
        {
            return 0;
        }
    }
    
    // Handle trailing asterisks
    while( *pattern == '*' )
    {
        pattern++;
    }
    
    return (*pattern == '\0' && *text == '\0');
}

/**
 * @brief Recursively search for files matching a pattern.
 * 
 * @param base_path Current directory path
 * @param name_pattern Pattern to match filenames against
 * @param found Pointer to counter for found files
 */
static void search_directory(const char* base_path, const char* name_pattern, int* found)
{
    void* dir = Dmod_OpenDir(base_path);
    if( dir == NULL )
    {
        return;
    }
    
    const char* entry;
    while( (entry = Dmod_ReadDir(dir)) != NULL )
    {
        // Skip . and ..
        if( strcmp(entry, ".") == 0 || strcmp(entry, "..") == 0 )
        {
            continue;
        }
        
        // Build full path
        size_t base_len = strlen(base_path);
        size_t entry_len = strlen(entry);
        size_t full_len = base_len + 1 + entry_len + 1;
        
        char* full_path = Dmod_Malloc(full_len);
        if( full_path == NULL )
        {
            continue;
        }
        
        if( base_path[base_len - 1] == '/' )
        {
            Dmod_SnPrintf(full_path, full_len, "%s%s", base_path, entry);
        }
        else
        {
            Dmod_SnPrintf(full_path, full_len, "%s/%s", base_path, entry);
        }
        
        // Check if entry matches the pattern
        if( match_pattern(name_pattern, entry) )
        {
            Dmod_Printf("%s\n", full_path);
            (*found)++;
        }
        
        // Try to recurse into subdirectories
        void* subdir = Dmod_OpenDir(full_path);
        if( subdir != NULL )
        {
            Dmod_CloseDir(subdir);
            search_directory(full_path, name_pattern, found);
        }
        
        Dmod_Free(full_path);
    }
    
    Dmod_CloseDir(dir);
}

/**
 * @brief Entry point for the 'find' command module.
 * 
 * Searches for files matching a pattern.
 * Usage: find <path> -name <pattern>
 * 
 * @param argc Number of arguments
 * @param argv Array of argument strings
 * @return int Exit code (0 on success, negative on error)
 */
int main( int argc, char** argv )
{
    const char* search_path = ".";
    const char* name_pattern = NULL;
    
    // Parse arguments
    for( int i = 1; i < argc; i++ )
    {
        if( strcmp(argv[i], "-name") == 0 )
        {
            if( i + 1 >= argc )
            {
                DMOD_LOG_ERROR("Option -name requires an argument\n");
                return -EINVAL;
            }
            name_pattern = argv[++i];
        }
        else if( argv[i][0] != '-' )
        {
            search_path = argv[i];
        }
        else
        {
            DMOD_LOG_ERROR("Unknown option: %s\n", argv[i]);
            return -EINVAL;
        }
    }
    
    if( name_pattern == NULL )
    {
        DMOD_LOG_ERROR("Usage: find <path> -name <pattern>\n");
        return -EINVAL;
    }
    
    int found = 0;
    search_directory(search_path, name_pattern, &found);
    
    return 0;
}
