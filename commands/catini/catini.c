#include <dmod.h>
#include <errno.h>
#include <string.h>

/**
 * VT100 color codes for syntax highlighting
 */
#define VT100_RESET     "\033[0m"
#define VT100_SECTION   "\033[1;36m"  // Bright Cyan for sections [section]
#define VT100_KEY       "\033[1;33m"  // Bright Yellow for keys
#define VT100_VALUE     "\033[0;32m"  // Green for values
#define VT100_COMMENT   "\033[0;90m"  // Dark Gray for comments

/**
 * @brief Simple whitespace check (space, tab, CR, LF)
 */
static inline int is_whitespace(char c)
{
    return (c == ' ' || c == '\t' || c == '\r' || c == '\n');
}

/**
 * @brief Check if a line is a comment (starts with ; or #)
 */
static int is_comment_line(const char* line)
{
    // Skip leading whitespace
    while (*line && is_whitespace(*line))
        line++;
    
    return (*line == ';' || *line == '#');
}

/**
 * @brief Check if a line is a section header [section]
 */
static int is_section_line(const char* line, const char** section_start, const char** section_end)
{
    // Skip leading whitespace
    while (*line && is_whitespace(*line))
        line++;
    
    if (*line != '[')
        return 0;
    
    *section_start = line;
    line++;
    
    // Find closing bracket
    while (*line && *line != ']' && *line != '\n' && *line != '\r')
        line++;
    
    if (*line == ']')
    {
        *section_end = line + 1;
        return 1;
    }
    
    return 0;
}

/**
 * @brief Check if a line is a key=value pair
 */
static int is_key_value_line(const char* line, const char** key_start, const char** key_end, 
                             const char** value_start)
{
    // Skip leading whitespace
    while (*line && is_whitespace(*line))
        line++;
    
    if (!*line || *line == '\n' || *line == '\r')
        return 0;
    
    *key_start = line;
    
    // Find the equals sign
    while (*line && *line != '=' && *line != '\n' && *line != '\r')
        line++;
    
    if (*line != '=')
        return 0;
    
    *key_end = line;
    line++; // Skip the '='
    
    *value_start = line;
    return 1;
}

/**
 * @brief Print a line with INI syntax highlighting
 */
static void print_highlighted_line(const char* line)
{
    const char* section_start;
    const char* section_end;
    const char* key_start;
    const char* key_end;
    const char* value_start;
    
    // Check for comment
    if (is_comment_line(line))
    {
        Dmod_Printf("%s%s%s", VT100_COMMENT, line, VT100_RESET);
        return;
    }
    
    // Check for section header
    if (is_section_line(line, &section_start, &section_end))
    {
        // Print leading whitespace
        const char* p = line;
        while (p < section_start)
        {
            Dmod_Printf("%c", *p);
            p++;
        }
        
        // Print section in color
        Dmod_Printf("%s", VT100_SECTION);
        while (section_start < section_end)
        {
            Dmod_Printf("%c", *section_start);
            section_start++;
        }
        Dmod_Printf("%s", VT100_RESET);
        
        // Print rest of line
        Dmod_Printf("%s", section_end);
        return;
    }
    
    // Check for key=value
    if (is_key_value_line(line, &key_start, &key_end, &value_start))
    {
        // Print leading whitespace
        const char* p = line;
        while (p < key_start)
        {
            Dmod_Printf("%c", *p);
            p++;
        }
        
        // Print key in color (without trailing whitespace)
        Dmod_Printf("%s", VT100_KEY);
        const char* key_p = key_start;
        while (key_p < key_end)
        {
            Dmod_Printf("%c", *key_p);
            key_p++;
        }
        Dmod_Printf("%s", VT100_RESET);
        
        // Print the equals sign
        Dmod_Printf("=");
        
        // Print value in color
        Dmod_Printf("%s%s%s", VT100_VALUE, value_start, VT100_RESET);
        return;
    }
    
    // Default: print line as-is (empty lines, etc.)
    Dmod_Printf("%s", line);
}

/**
 * @brief Entry point for the 'catini' command module.
 * 
 * Displays INI file contents with syntax highlighting using VT100 codes.
 * Usage: catini <file1> [file2 ...]
 * 
 * @param argc Number of arguments
 * @param argv Array of argument strings
 * @return int Exit code (0 on success, negative on error)
 */
int main(int argc, char** argv)
{
    if (argc < 2)
    {
        DMOD_LOG_ERROR("Usage: catini <file1> [file2 ...]\n");
        return -EINVAL;
    }

    int result = 0;
    for (int i = 1; i < argc; i++)
    {
        const char* file_name = argv[i];
        void* file = Dmod_FileOpen(file_name, "r");
        if (file == NULL)
        {
            DMOD_LOG_ERROR("Failed to open file '%s'\n", file_name);
            result = -1;
            continue;
        }

        char buffer[4096];
        size_t bytes_read;
        char line_buffer[4096];
        int line_pos = 0;
        
        while ((bytes_read = Dmod_FileRead(buffer, 1, sizeof(buffer) - 1, file)) > 0)
        {
            buffer[bytes_read] = '\0';
            
            for (size_t j = 0; j < bytes_read; j++)
            {
                char c = buffer[j];
                
                if (c == '\n' || c == '\r')
                {
                    // End of line - print it with highlighting
                    line_buffer[line_pos] = c;
                    line_buffer[line_pos + 1] = '\0';
                    print_highlighted_line(line_buffer);
                    line_pos = 0;
                    
                    // Handle \r\n by skipping the \n if next char is \n
                    if (c == '\r' && j + 1 < bytes_read && buffer[j + 1] == '\n')
                    {
                        j++;
                        Dmod_Printf("\n");
                    }
                }
                else
                {
                    // Add character to line buffer
                    if (line_pos < sizeof(line_buffer) - 2)
                    {
                        line_buffer[line_pos++] = c;
                    }
                }
            }
        }
        
        // Print any remaining content in the line buffer
        if (line_pos > 0)
        {
            line_buffer[line_pos] = '\0';
            print_highlighted_line(line_buffer);
        }

        Dmod_FileClose(file);
    }

    return result;
}
