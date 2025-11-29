#include <dmod.h>
#include <errno.h>
#include <string.h>

/**
 * @brief Simple pattern matching function for grep.
 * 
 * @param text Text to search in
 * @param pattern Pattern to search for
 * @param ignore_case Whether to ignore case
 * @return const char* Pointer to first match, or NULL if not found
 */
static const char* find_pattern(const char* text, const char* pattern, int ignore_case)
{
    if( text == NULL || pattern == NULL )
    {
        return NULL;
    }

    size_t text_len = strlen(text);
    size_t pattern_len = strlen(pattern);
    
    if( pattern_len == 0 )
    {
        return text;
    }
    
    if( pattern_len > text_len )
    {
        return NULL;
    }

    for( size_t i = 0; i <= text_len - pattern_len; i++ )
    {
        int match = 1;
        for( size_t j = 0; j < pattern_len; j++ )
        {
            char c1 = text[i + j];
            char c2 = pattern[j];
            
            if( ignore_case )
            {
                // Simple case conversion for ASCII
                if( c1 >= 'A' && c1 <= 'Z' ) c1 = c1 - 'A' + 'a';
                if( c2 >= 'A' && c2 <= 'Z' ) c2 = c2 - 'A' + 'a';
            }
            
            if( c1 != c2 )
            {
                match = 0;
                break;
            }
        }
        
        if( match )
        {
            return &text[i];
        }
    }

    return NULL;
}

/**
 * @brief Entry point for the 'grep' command module.
 * 
 * Searches for patterns in files.
 * Usage: grep [-i] [-n] [-v] <pattern> <file1> [file2 ...]
 * 
 * @param argc Number of arguments
 * @param argv Array of argument strings
 * @return int Exit code (0 on match found, 1 on no match, negative on error)
 */
int main( int argc, char** argv )
{
    int ignore_case = 0;
    int show_line_numbers = 0;
    int invert_match = 0;
    const char* pattern = NULL;
    int first_file_index = -1;
    
    // Parse arguments
    for( int i = 1; i < argc; i++ )
    {
        if( argv[i][0] == '-' && pattern == NULL )
        {
            // Handle flags
            for( int j = 1; argv[i][j] != '\0'; j++ )
            {
                switch( argv[i][j] )
                {
                    case 'i':
                        ignore_case = 1;
                        break;
                    case 'n':
                        show_line_numbers = 1;
                        break;
                    case 'v':
                        invert_match = 1;
                        break;
                    default:
                        DMOD_LOG_ERROR("Unknown option: -%c\n", argv[i][j]);
                        return -EINVAL;
                }
            }
        }
        else if( pattern == NULL )
        {
            pattern = argv[i];
        }
        else if( first_file_index < 0 )
        {
            first_file_index = i;
        }
    }

    if( pattern == NULL || first_file_index < 0 )
    {
        DMOD_LOG_ERROR("Usage: grep [-i] [-n] [-v] <pattern> <file1> [file2 ...]\n");
        return -EINVAL;
    }

    int match_found = 0;
    int num_files = argc - first_file_index;
    int show_filename = (num_files > 1);

    for( int i = first_file_index; i < argc; i++ )
    {
        const char* file_path = argv[i];
        void* file = Dmod_FileOpen(file_path, "r");
        
        if( file == NULL )
        {
            DMOD_LOG_ERROR("Failed to open file '%s'\n", file_path);
            continue;
        }

        char buffer[4096];
        int line_number = 0;
        
        while( Dmod_FileReadLine(buffer, sizeof(buffer), file) != NULL )
        {
            line_number++;
            
            const char* match = find_pattern(buffer, pattern, ignore_case);
            int should_print = invert_match ? (match == NULL) : (match != NULL);
            
            if( should_print )
            {
                match_found = 1;
                
                if( show_filename )
                {
                    Dmod_Printf("%s:", file_path);
                }
                
                if( show_line_numbers )
                {
                    Dmod_Printf("%d:", line_number);
                }
                
                Dmod_Printf("%s", buffer);
                
                // Add newline if line doesn't end with one
                size_t len = strlen(buffer);
                if( len == 0 || buffer[len - 1] != '\n' )
                {
                    Dmod_Printf("\n");
                }
            }
        }

        Dmod_FileClose(file);
    }

    return match_found ? 0 : 1;
}
