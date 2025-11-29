#include <dmod.h>
#include <errno.h>
#include <string.h>

/**
 * @brief Simple string to integer conversion.
 * 
 * @param str String to convert
 * @return int Converted integer value
 */
static int simple_atoi(const char* str)
{
    int result = 0;
    int sign = 1;
    
    if( str == NULL )
    {
        return 0;
    }
    
    // Handle negative numbers
    if( *str == '-' )
    {
        sign = -1;
        str++;
    }
    else if( *str == '+' )
    {
        str++;
    }
    
    while( *str >= '0' && *str <= '9' )
    {
        result = result * 10 + (*str - '0');
        str++;
    }
    
    return result * sign;
}

/**
 * @brief Entry point for the 'tail' command module.
 * 
 * Displays the last lines of a file.
 * Usage: tail [-n <lines>] <file>
 * 
 * @param argc Number of arguments
 * @param argv Array of argument strings
 * @return int Exit code (0 on success, negative on error)
 */
int main( int argc, char** argv )
{
    int num_lines = 10;  // Default number of lines
    const char* file_path = NULL;
    
    // Parse arguments
    for( int i = 1; i < argc; i++ )
    {
        if( strcmp(argv[i], "-n") == 0 )
        {
            if( i + 1 >= argc )
            {
                DMOD_LOG_ERROR("Option -n requires an argument\n");
                return -EINVAL;
            }
            num_lines = simple_atoi(argv[++i]);
            if( num_lines <= 0 )
            {
                DMOD_LOG_ERROR("Invalid number of lines: %s\n", argv[i]);
                return -EINVAL;
            }
        }
        else if( argv[i][0] == '-' && argv[i][1] != '\0' )
        {
            // Check for -<number> format (e.g., -20)
            int val = simple_atoi(&argv[i][1]);
            if( val > 0 )
            {
                num_lines = val;
            }
            else
            {
                DMOD_LOG_ERROR("Unknown option: %s\n", argv[i]);
                return -EINVAL;
            }
        }
        else
        {
            file_path = argv[i];
        }
    }

    if( file_path == NULL )
    {
        DMOD_LOG_ERROR("Usage: tail [-n <lines>] <file>\n");
        return -EINVAL;
    }

    void* file = Dmod_FileOpen(file_path, "r");
    if( file == NULL )
    {
        DMOD_LOG_ERROR("Failed to open file '%s'\n", file_path);
        return -1;
    }

    // First, count total lines in file
    char buffer[4096];
    int total_lines = 0;
    int last_line_had_newline = 1;
    
    while( Dmod_FileReadLine(buffer, sizeof(buffer), file) != NULL )
    {
        size_t len = strlen(buffer);
        if( len > 0 )
        {
            if( buffer[len - 1] == '\n' )
            {
                total_lines++;
                last_line_had_newline = 1;
            }
            else
            {
                // Line without newline (possibly last line)
                last_line_had_newline = 0;
            }
        }
    }
    
    // Count last line if it didn't end with newline
    if( !last_line_had_newline )
    {
        total_lines++;
    }

    // Calculate start line
    int start_line = total_lines - num_lines;
    if( start_line < 0 )
    {
        start_line = 0;
    }

    // Seek back to beginning and skip to start line
    Dmod_FileSeek(file, 0, DMOD_SEEK_SET);
    
    int current_line = 0;
    while( current_line < start_line )
    {
        char* line = Dmod_FileReadLine(buffer, sizeof(buffer), file);
        if( line == NULL )
        {
            break;
        }
        // Each successful read is a line (whether it ends with newline or not)
        current_line++;
    }

    // Print remaining lines
    while( Dmod_FileReadLine(buffer, sizeof(buffer), file) != NULL )
    {
        Dmod_Printf("%s", buffer);
    }

    Dmod_FileClose(file);
    return 0;
}
