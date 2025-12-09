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
                // Erase character from terminal if echo is enabled
                // When echo is disabled, no visual feedback is needed
                uint32_t flags = Dmod_Stdin_GetFlags();
                if( flags & DMOD_STDIN_FLAG_ECHO )
                {
                    Dmod_Printf("\b \b");
                }
            }
        }
        else
        {
            buffer[position] = (char)c;
            position++;
            if( position >= buffer_size )
            {
                buffer_size *= 2;
                char* new_buffer = Dmod_Realloc( buffer, buffer_size );
                if( new_buffer == NULL )
                {
                    DMOD_LOG_ERROR("Memory allocation failed in read_line during buffer resize\n");
                    Dmod_Free( buffer );
                    return NULL;
                }
                buffer = new_buffer;
            }
        }
    }

    if( out_len != NULL )
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