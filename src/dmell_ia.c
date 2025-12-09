#include <errno.h>
#include <string.h>
#include "dmell_ia.h"
#include "dmod.h"
#include "dmell_script.h"
#include "dmell_cmd.h"

// Maximum length for word completion buffers
#define MAX_COMPLETION_WORD_LEN 256

// External references to registered commands
extern dmell_cmd_t* g_registered_commands;
extern size_t g_registered_command_count;

/**
 * @brief Find matching built-in command name.
 * 
 * @param partial_name Partial command name to match
 * @param out_match Output buffer for the matching command name
 * @param max_length Maximum length of the output buffer
 * @return true if a match was found, false otherwise
 */
static bool find_builtin_command_match(const char* partial_name, char* out_match, size_t max_length)
{
    if( partial_name == NULL || out_match == NULL || max_length == 0 )
    {
        return false;
    }

    size_t partial_len = strlen(partial_name);
    if( partial_len == 0 || partial_len >= MAX_COMPLETION_WORD_LEN )
    {
        return false;
    }

    // Search through registered built-in commands
    for( size_t i = 0; i < g_registered_command_count; i++ )
    {
        const char* cmd_name = g_registered_commands[i].name;
        if( cmd_name != NULL && strncmp(cmd_name, partial_name, partial_len) == 0 )
        {
            size_t cmd_len = strlen(cmd_name);
            if( cmd_len + 1 <= max_length )
            {
                strncpy(out_match, cmd_name, max_length - 1);
                out_match[max_length - 1] = '\0';
                return true;
            }
        }
    }

    return false;
}

/**
 * @brief Helper function to print the command prompt.
 */
static void print_prompt()
{
    const char* host_name = Dmod_GetEnv( "HOSTNAME" );
    host_name = ( host_name != NULL ) ? host_name : "dmell";
    char cwd[1024] = {0};
    Dmod_GetCwd( cwd, sizeof(cwd) );
    Dmod_Printf("\033[35;1m%s\033[37;1m@\033[34;1m%s\033[0m> ", host_name, cwd);
}

/**
 * @brief Find matching file or directory name, supporting paths.
 * 
 * @param partial_name Partial file name or path to match
 * @param out_match Output buffer for the matching name (full path if input contained path)
 * @param max_length Maximum length of the output buffer
 * @return true if a match was found, false otherwise
 */
static bool find_file_match(const char* partial_name, char* out_match, size_t max_length)
{
    if( partial_name == NULL || out_match == NULL || max_length == 0 )
    {
        return false;
    }

    // Validate that partial_name is a valid null-terminated string
    // (check first MAX_COMPLETION_WORD_LEN chars to prevent buffer over-read)
    size_t partial_len = 0;
    for( size_t i = 0; i < MAX_COMPLETION_WORD_LEN; i++ )
    {
        if( partial_name[i] == '\0' )
        {
            partial_len = i;
            break;
        }
    }
    
    if( partial_len == 0 || partial_len == MAX_COMPLETION_WORD_LEN )
    {
        return false; // Empty string or not null-terminated within bounds
    }

    // Parse the partial path to extract directory and filename parts
    const char* last_slash = NULL;
    for( size_t i = 0; i < partial_len; i++ )
    {
        if( partial_name[i] == '/' )
        {
            last_slash = partial_name + i;
        }
    }

    char search_dir[1024] = {0};
    const char* partial_filename;
    size_t partial_filename_len;
    
    if( last_slash != NULL )
    {
        // Path contains a slash - extract directory and filename parts
        size_t dir_len = last_slash - partial_name + 1; // Include the slash
        if( dir_len + 1 > sizeof(search_dir) )
        {
            return false; // Directory path too long (need space for null terminator)
        }
        
        strncpy(search_dir, partial_name, dir_len);
        search_dir[dir_len] = '\0';
        
        // Handle the case where the path is just "/"
        if( dir_len == 1 && search_dir[0] == '/' )
        {
            // Keep it as "/"
        }
        else
        {
            // Remove trailing slash for directory opening
            search_dir[dir_len - 1] = '\0';
        }
        
        partial_filename = last_slash + 1;
        partial_filename_len = partial_len - (last_slash - partial_name + 1);
    }
    else
    {
        // No slash - search in current directory
        Dmod_GetCwd(search_dir, sizeof(search_dir));
        partial_filename = partial_name;
        partial_filename_len = partial_len;
    }
    
    void* dir = Dmod_OpenDir(search_dir);
    if( dir == NULL )
    {
        return false;
    }
    
    const char* entry;
    bool found = false;
    
    while( (entry = Dmod_ReadDir(dir)) != NULL )
    {
        // Skip "." and ".." entries
        if( strcmp(entry, ".") == 0 || strcmp(entry, "..") == 0 )
        {
            continue;
        }
        
        // Check if entry starts with the partial filename
        if( strncmp(entry, partial_filename, partial_filename_len) == 0 )
        {
            // Build the full match path
            if( last_slash != NULL )
            {
                // Reconstruct the path with the directory prefix
                size_t dir_prefix_len = last_slash - partial_name + 1;
                size_t entry_len = strlen(entry);
                
                if( dir_prefix_len + entry_len + 1 <= max_length )
                {
                    // Safely construct the full path using snprintf
                    int written = Dmod_SnPrintf(out_match, max_length, "%.*s%s", 
                                                (int)dir_prefix_len, partial_name, entry);
                    if( written > 0 && (size_t)written < max_length )
                    {
                        // Ensure null termination
                        out_match[written] = '\0';
                        found = true;
                        break;
                    }
                }
            }
            else
            {
                // No path prefix, just return the filename
                size_t entry_len = strlen(entry);
                if( entry_len + 1 <= max_length )
                {
                    strncpy(out_match, entry, max_length - 1);
                    out_match[max_length - 1] = '\0';
                    found = true;
                    break;
                }
            }
        }
    }
    
    Dmod_CloseDir(dir);
    return found;
}

/**
 * @brief Handle tab completion for the current input buffer.
 * 
 * This function attempts to complete the current word in the buffer by:
 * 1. For the first word (command position):
 *    a. First trying to match against built-in commands
 *    b. Then trying to match against module names using Dmod_FindMatch
 * 2. For all words, falling back to file/directory completion if no command match
 * 
 * @param buffer The input buffer
 * @param position Current position in the buffer
 * @param buffer_size Current size of the buffer
 * @param should_echo Whether to echo the completion to the terminal
 * @return size_t New position in the buffer after completion
 */
static size_t handle_tab_completion(char* buffer, size_t position, size_t buffer_size, bool should_echo)
{
    if( buffer == NULL )
    {
        return position;
    }

    // Find the start of the current word (after the last space or beginning of buffer)
    size_t word_start = position;
    while( word_start > 0 && buffer[word_start - 1] != ' ' && buffer[word_start - 1] != '\t' )
    {
        word_start--;
    }

    // Extract the partial word
    size_t word_len = position - word_start;
    if( word_len == 0 )
    {
        return position;
    }

    char partial_word[MAX_COMPLETION_WORD_LEN];
    if( word_len >= sizeof(partial_word) )
    {
        return position; // Word too long
    }
    
    strncpy(partial_word, buffer + word_start, word_len);
    partial_word[word_len] = '\0';

    char match[MAX_COMPLETION_WORD_LEN] = {0};
    bool found = false;
    
    // Determine if we're completing the first word (command) or subsequent words (arguments)
    // Check if all characters before word_start are whitespace
    bool is_first_word = true;
    for( size_t i = 0; i < word_start; i++ )
    {
        if( buffer[i] != ' ' && buffer[i] != '\t' )
        {
            is_first_word = false;
            break;
        }
    }
    
    if( is_first_word )
    {
        // For the first word, prioritize built-in commands, then modules
        found = find_builtin_command_match(partial_word, match, sizeof(match));
        
        if( !found )
        {
            found = Dmod_FindMatch(partial_word, match, sizeof(match));
        }
    }
    
    // For subsequent words or if no command match found, try file completion
    if( !found )
    {
        found = find_file_match(partial_word, match, sizeof(match));
    }

    if( found )
    {
        size_t match_len = strlen(match);
        
        // Verify that the match is actually longer than our partial word
        if( match_len <= word_len )
        {
            return position;
        }
        
        size_t completion_len = match_len - word_len;
        
        // Check if there's enough space in the buffer
        if( position + completion_len >= buffer_size )
        {
            return position;
        }

        // Append the completion to the buffer
        for( size_t i = 0; i < completion_len; i++ )
        {
            buffer[position] = match[word_len + i];
            if( should_echo )
            {
                Dmod_Printf("%c", buffer[position]);
            }
            position++;
        }
    }

    return position;
}

/**
 * @brief Helper function to read a line of input from the user.
 * 
 * This function reads characters from stdin until a newline or EOF is encountered.
 * It temporarily disables terminal echo to manually control character display and
 * supports backspace handling (ASCII 127 DEL and 8 BS) using VT100 escape sequences
 * for proper visual feedback. Original echo settings are restored after reading.
 * 
 * @param out_len Output parameter to hold the length of the read line
 * @return char* The read line, or NULL on failure
 */
static char* read_line( size_t* out_len )
{
    print_prompt();

    size_t buffer_size = 256;
    char* buffer = Dmod_Malloc( buffer_size );
    if( buffer == NULL )
    {
        DMOD_LOG_ERROR("Memory allocation failed in read_line\n");
        return NULL;
    }

    // Save original stdin flags and disable echo to handle input manually
    uint32_t original_flags = Dmod_Stdin_GetFlags();
    uint32_t new_flags = (original_flags & ~(DMOD_STDIN_FLAG_ECHO|DMOD_STDIN_FLAG_CANONICAL));
    Dmod_Stdin_SetFlags(new_flags);
    bool should_echo = (original_flags & DMOD_STDIN_FLAG_ECHO) != 0;

    size_t position = 0;
    while( true )
    {
        int c = Dmod_Getc();
        if( c == EOF || c == '\n' )
        {
            buffer[position] = '\0';
            Dmod_Printf("\n");
            break;
        }
        else if( c == 9 ) // Tab character
        {
            position = handle_tab_completion(buffer, position, buffer_size, should_echo);
        }
        else if( c == 127 || c == 8 ) // Backspace (DEL or BS)
        {
            if( position > 0 )
            {
                position--;
                // Erase character from terminal using VT100 sequences
                // Move cursor back, write space, move cursor back again
                if( should_echo )
                {
                    Dmod_Printf("\033[1D \033[1D");
                }
            }
        }
        else
        {
            buffer[position] = (char)c;
            // Manually echo printable characters if original echo was enabled
            // Only echo printable ASCII characters (32-126) to avoid terminal corruption
            if( should_echo && c >= 32 && c <= 126 )
            {
                Dmod_Printf("%c", (char)c);
            }
            position++;
            // Check if buffer needs to be expanded before next write
            // This ensures buffer[position] is always valid for the next iteration
            if( position >= buffer_size )
            {
                buffer_size *= 2;
                char* new_buffer = Dmod_Realloc( buffer, buffer_size );
                if( new_buffer == NULL )
                {
                    DMOD_LOG_ERROR("Memory allocation failed in read_line during buffer resize\n");
                    Dmod_Free( buffer );
                    buffer = NULL;
                    goto cleanup;
                }
                buffer = new_buffer;
            }
        }
    }

cleanup:
    // Restore original stdin flags
    Dmod_Stdin_SetFlags(original_flags);

    if( buffer != NULL && out_len != NULL )
    {
        *out_len = position;
    }
    return buffer;
}

/**
 * @brief Enters interactive mode for command input.
 * 
 * @return int Exit code
 */
int dmell_interactive_mode( void )
{
    while( true )
    {
        size_t line_len = 0;
        char* line = read_line( &line_len );
        if( line == NULL )
        {
            DMOD_LOG_ERROR("Failed to read line in interactive mode\n");
            return -ENOMEM;
        }
        if(strncmp(line, "exit", 4) == 0 || strncmp(line, "quit", 4) == 0)
        {
            Dmod_Free( line );
            break;
        }

        int exit_code = dmell_run_script_line(&g_dmell_global_script_ctx, line, line_len );
        Dmod_Free( line );
    }
    return 0;
}