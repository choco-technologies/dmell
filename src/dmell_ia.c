#include <errno.h>
#include <string.h>
#include "dmell_ia.h"
#include "dmod.h"
#include "dmell_script.h"

/**
 * @brief Helper function to print the command prompt.
 */
static void print_prompt()
{
    const char* host_name = Dmod_GetEnv( "HOSTNAME" );
    host_name = ( host_name != NULL ) ? host_name : "dmell";
    char cwd[256] = {0};
    Dmod_GetCwd( cwd, sizeof(cwd) );
    Dmod_Printf("\033[35;1m%s\033[37;1m@\033[34;1m%s\033[0m> ", host_name, cwd);
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
    Dmod_Stdin_SetFlags(original_flags & ~DMOD_STDIN_FLAG_ECHO);
    bool should_echo = (original_flags & DMOD_STDIN_FLAG_ECHO) != 0;

    size_t position = 0;
    while( true )
    {
        int c = Dmod_Getc();
        if( c == EOF || c == '\n' )
        {
            buffer[position] = '\0';
            break;
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