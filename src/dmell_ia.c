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
    char* cwd = Dmod_Malloc( 256 );
    if( cwd == NULL )
    {
        Dmod_Printf("\033[35;1m%s\033[0m> ", host_name);
        return;
    }
    cwd[0] = '\0';
    Dmod_GetCwd( cwd, 256 );
    Dmod_Printf("\033[35;1m%s\033[37;1m@\033[34;1m%s\033[0m> ", host_name, cwd);
    Dmod_Free( cwd );
}

/**
 * @brief Parse a partial file path into its directory and filename components.
 * 
 * @param partial_name The partial path to parse (must be non-NULL)
 * @param out_search_dir Output buffer for the directory to search in (at least 256 bytes)
 * @param out_partial_filename Output pointer to the filename part within partial_name
 * @param out_partial_filename_len Output length of the filename part
 * @param out_dir_prefix_len Output length of the directory prefix (including trailing slash)
 * @return true on success, false on failure
 */
static bool parse_partial_path(
    const char* partial_name,
    char* out_search_dir,
    const char** out_partial_filename,
    size_t* out_partial_filename_len,
    size_t* out_dir_prefix_len)
{
    size_t partial_len = strlen(partial_name);

    // Find the last '/' in partial_name
    const char* last_slash = NULL;
    for( size_t i = 0; i < partial_len; i++ )
    {
        if( partial_name[i] == '/' )
        {
            last_slash = partial_name + i;
        }
    }

    if( last_slash != NULL )
    {
        size_t dir_len = (size_t)(last_slash - partial_name) + 1; // include the slash
        if( dir_len + 1 > 256 )
        {
            return false;
        }

        strncpy(out_search_dir, partial_name, dir_len);
        out_search_dir[dir_len] = '\0';

        // Remove trailing slash unless it is the root "/"
        if( !( dir_len == 1 && out_search_dir[0] == '/' ) )
        {
            out_search_dir[dir_len - 1] = '\0';
        }

        *out_partial_filename     = last_slash + 1;
        *out_partial_filename_len = partial_len - dir_len;
        *out_dir_prefix_len       = dir_len;
    }
    else
    {
        // No slash – search in current working directory
        Dmod_GetCwd(out_search_dir, 256);
        *out_partial_filename     = partial_name;
        *out_partial_filename_len = partial_len;
        *out_dir_prefix_len       = 0;
    }

    return true;
}

/**
 * @brief Find all matching file/directory entries for a partial path and compute
 *        the longest common prefix.
 * 
 * Uses Dmod_ReadDirEx to detect directory entries so that a trailing '/' can be
 * appended when there is exactly one match and it is a directory.  When there are
 * multiple matches the common prefix of all entry names is computed.
 * 
 * @param partial_name Partial file name or path to match
 * @param out_match    Output buffer for the common-prefix match (full path)
 * @param max_length   Maximum length of the output buffer
 * @return Number of matching entries (0 = none, 1 = unique, >1 = multiple)
 */
static int find_file_matches(const char* partial_name, char* out_match, size_t max_length)
{
    if( partial_name == NULL || out_match == NULL || max_length == 0 )
    {
        return 0;
    }

    memset(out_match, 0, max_length);

    size_t partial_len = strlen(partial_name);
    if( partial_len >= MAX_COMPLETION_WORD_LEN )
    {
        return 0;
    }

    char* search_dir = Dmod_Malloc(256);
    if( search_dir == NULL )
    {
        return 0;
    }
    search_dir[0] = '\0';

    const char* partial_filename;
    size_t partial_filename_len;
    size_t dir_prefix_len;

    if( !parse_partial_path(partial_name, search_dir, &partial_filename, &partial_filename_len, &dir_prefix_len) )
    {
        Dmod_Free(search_dir);
        return 0;
    }

    void* dir = Dmod_OpenDir(search_dir);
    Dmod_Free(search_dir);
    if( dir == NULL )
    {
        return 0;
    }

    char common_prefix[MAX_COMPLETION_WORD_LEN];
    common_prefix[0] = '\0';
    size_t common_prefix_len = 0;
    int match_count = 0;

    const Dmod_DirEntry_t* entry;
    while( (entry = Dmod_ReadDirEx(dir)) != NULL )
    {
        if( strcmp(entry->name, ".") == 0 || strcmp(entry->name, "..") == 0 )
        {
            continue;
        }

        if( strncmp(entry->name, partial_filename, partial_filename_len) == 0 )
        {
            size_t entry_name_len = strlen(entry->name);
            if( entry_name_len >= MAX_COMPLETION_WORD_LEN )
            {
                continue; // Entry name too long to handle
            }

            match_count++;

            if( match_count == 1 )
            {
                // First match: initialize common prefix
                strncpy(common_prefix, entry->name, MAX_COMPLETION_WORD_LEN - 1);
                common_prefix[MAX_COMPLETION_WORD_LEN - 1] = '\0';
                common_prefix_len = entry_name_len;
            }
            else
            {
                // Narrow the common prefix to what is shared with this entry
                size_t i = 0;
                while( i < common_prefix_len && common_prefix[i] == entry->name[i] )
                {
                    i++;
                }
                common_prefix_len            = i;
                common_prefix[common_prefix_len] = '\0';
            }
        }
    }

    Dmod_CloseDir(dir);

    if( match_count == 0 )
    {
        return 0;
    }

    // For a single match, verify it is an actual directory by attempting to open
    // it - do not rely on entry->type which may be unreliable on virtual filesystems.
    bool append_slash = false;
    if( match_count == 1 )
    {
        // Construct the full path: directory prefix + matched entry name
        char full_path[MAX_COMPLETION_WORD_LEN * 2];
        size_t full_path_len = dir_prefix_len + common_prefix_len;
        if( full_path_len + 1 <= sizeof(full_path) )
        {
            if( dir_prefix_len > 0 )
            {
                strncpy(full_path, partial_name, dir_prefix_len);
            }
            strncpy(full_path + dir_prefix_len, common_prefix, common_prefix_len);
            full_path[full_path_len] = '\0';

            void* verify_dir = Dmod_OpenDir(full_path);
            if( verify_dir != NULL )
            {
                Dmod_CloseDir(verify_dir);
                append_slash = true;
            }
        }
    }

    size_t suffix_len = common_prefix_len + ( append_slash ? 1 : 0 );

    if( dir_prefix_len + suffix_len + 1 > max_length )
    {
        return 0;
    }

    if( dir_prefix_len > 0 )
    {
        strncpy(out_match, partial_name, dir_prefix_len);
    }
    strncpy(out_match + dir_prefix_len, common_prefix, common_prefix_len);
    if( append_slash )
    {
        out_match[dir_prefix_len + common_prefix_len]     = '/';
        out_match[dir_prefix_len + common_prefix_len + 1] = '\0';
    }
    else
    {
        out_match[dir_prefix_len + common_prefix_len] = '\0';
    }

    return match_count;
}

/**
 * @brief Print all matching file/directory entries for a partial path.
 * 
 * Entries that are directories are printed with a trailing '/'.
 * Entries are separated by two spaces.
 * A trailing newline is printed after the last entry.
 * 
 * @param partial_name Partial file name or path to match
 */
static void print_file_matches(const char* partial_name)
{
    if( partial_name == NULL )
    {
        return;
    }

    size_t partial_len = strlen(partial_name);
    if( partial_len >= MAX_COMPLETION_WORD_LEN )
    {
        return;
    }

    char* search_dir = Dmod_Malloc(256);
    if( search_dir == NULL )
    {
        return;
    }
    search_dir[0] = '\0';

    const char* partial_filename;
    size_t partial_filename_len;
    size_t dir_prefix_len;

    if( !parse_partial_path(partial_name, search_dir, &partial_filename, &partial_filename_len, &dir_prefix_len) )
    {
        Dmod_Free(search_dir);
        return;
    }

    void* dir = Dmod_OpenDir(search_dir);
    Dmod_Free(search_dir);
    if( dir == NULL )
    {
        return;
    }

    bool printed_any = false;
    const Dmod_DirEntry_t* entry;
    while( (entry = Dmod_ReadDirEx(dir)) != NULL )
    {
        if( strcmp(entry->name, ".") == 0 || strcmp(entry->name, "..") == 0 )
        {
            continue;
        }

        if( strncmp(entry->name, partial_filename, partial_filename_len) == 0 )
        {
            if( printed_any )
            {
                Dmod_Printf("  ");
            }
            Dmod_Printf("%s", entry->name);
            if( entry->type == Dmod_DirEntryType_Dir )
            {
                Dmod_Printf("/");
            }
            printed_any = true;
        }
    }

    if( printed_any )
    {
        Dmod_Printf("\n");
    }

    Dmod_CloseDir(dir);
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
    if( word_len >= MAX_COMPLETION_WORD_LEN )
    {
        return position; // Word too long
    }
    
    strncpy(partial_word, buffer + word_start, word_len);
    partial_word[word_len] = '\0';

    char* match = Dmod_Malloc( MAX_COMPLETION_WORD_LEN );
    if( match == NULL )
    {
        return position;
    }
    match[0] = '\0';
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
        found = find_builtin_command_match(partial_word, match, MAX_COMPLETION_WORD_LEN);
        
        if( !found )
        {
            found = Dmod_FindMatch(partial_word, match, MAX_COMPLETION_WORD_LEN);
        }
    }
    
    // For subsequent words or if no command match found, try file completion
    int file_match_count = 0;
    if( !found )
    {
        file_match_count = find_file_matches(partial_word, match, MAX_COMPLETION_WORD_LEN);
        found = ( file_match_count > 0 );
    }

    if( found )
    {
        size_t match_len = strlen(match);

        if( match_len > word_len )
        {
            // There are additional characters to append
            size_t completion_len = match_len - word_len;

            // Check if there's enough space in the buffer
            if( position + completion_len >= buffer_size )
            {
                Dmod_Free( match );
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
        else if( file_match_count > 1 && should_echo )
        {
            // No further prefix extension is possible – list all matching entries
            // so the user can see the available options (Linux-like behaviour).
            Dmod_Printf("\n");
            print_file_matches(partial_word);
            // Reprint the prompt and the buffer contents so the user can continue
            print_prompt();
            buffer[position] = '\0';
            Dmod_Printf("%s", buffer);
        }
    }

    Dmod_Free( match );
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