#include "dmell_cmd.h"
#include <dmod.h>
#include <string.h>
#include <errno.h>

/**
 * @brief Helper function to skip whitespaces in a command string.
 * 
 * @param str Current position in the command string
 * @param end_ptr Pointer to the end of the command string
 * @return const char* Pointer to the first non-whitespace character
 */
static const char* skip_whitespaces( const char* str, const char* end_ptr )
{
    const char* ptr = str;
    while(ptr < end_ptr && (*ptr == ' ' || *ptr == '\t') )
    {
        ptr++;
    }
    return ptr;
}

/**
 * @brief Helper function to find the next argument in a command string.
 * 
 * @param str Current position in the command string
 * @param end_ptr Pointer to the end of the command string
 * @return const char* Pointer to the start of the next argument, or NULL if none found
 */
static const char* get_next_arg( const char* str, const char* end_ptr)
{
    const char* ptr = str;

    bool quote_active = false;
    char quote_char = '\0';
    while( ptr < end_ptr )
    {
        if( !quote_active && (*ptr == '"' || *ptr == '\'') )
        {
            quote_active = true;
            quote_char = *ptr;
        }
        else if( quote_active && *ptr == quote_char )
        {
            quote_active = false;
            quote_char = '\0';
        }
        else if( !quote_active && (*ptr == ' ' || *ptr == '\t') )
        {
            break;
        }
        ptr++;
    }

    if( ptr >= end_ptr )
    {
        return NULL;
    }

    return ptr;
}

/**
 * @brief Helper function to duplicate an argument string.
 * 
 * @param arg Argument string to duplicate
 * @param len Length of the argument string
 * @return char* Duplicated argument string, or NULL on failure
 */
static char* duplicate_arg( const char* arg, size_t len )
{
    char* new_arg = Dmod_Malloc( len + 1 );
    if( new_arg == NULL )
    {
        return NULL;
    }
    memcpy( new_arg, arg, len );
    new_arg[len] = '\0';
    return new_arg;
}

/**
 * @brief Helper function to add an argument to the dmell_argv_t structure.
 * 
 * @param argv Pointer to the dmell_argv_t structure
 * @param arg Argument string to add
 * @param next_arg Pointer to the start of the next argument
 * @param end_ptr Pointer to the end of the command string
 * @return int 0 on success, negative value on error
 */
static int add_arg( dmell_argv_t* argv, const char* arg, const char* next_arg, const char* end_ptr )
{
    const char* arg_end = (next_arg != NULL) ? next_arg : end_ptr;
    size_t arg_len = arg_end - arg;
    char* new_arg = duplicate_arg( arg, arg_len );
    if( new_arg == NULL )
    {
        DMOD_LOG_ERROR("Memory allocation failed in duplicate_arg\n");
        return -ENOMEM;
    }

    argv->argv = Dmod_Realloc( argv->argv, sizeof(char*) * (argv->argc + 1) );
    if( argv->argv == NULL )
    {
        Dmod_Free( new_arg );
        DMOD_LOG_ERROR("Memory allocation failed in Dmod_Realloc\n");
        return -ENOMEM;
    }

    argv->argv[argv->argc] = new_arg;
    argv->argc += 1;
    return 0;
}

/**
 * @brief Helper function to free the dmell_argv_t structure.
 * 
 * @param argv Pointer to the dmell_argv_t structure to free
 */
static void free_dmell_argv( dmell_argv_t* argv )
{
    if( argv == NULL )
    {
        return;
    }

    for( int i = 0; i < argv->argc; i++ )
    {
        Dmod_Free( argv->argv[i] );
    }
    Dmod_Free( argv->argv );
    argv->argc = 0;
    argv->argv = NULL;
}


/**
 * @brief Runs a command by its name.
 * 
 * @param cmd_name Name of the command to run
 * @param argc Number of arguments
 * @param argv Array of arguments
 * @return int Exit code of the command
 */
int dmell_run_command(const char* cmd_name, int argc, char** argv)
{
    return -1;
}

/**
 * @brief Parses a command string into arguments.
 * 
 * @param cmd Command string to parse
 * @param len Length of the command string
 * @param out_argv Output structure to hold parsed arguments
 * @return int 0 on success, negative value on error
 */
int dmell_parse_command( const char* cmd, size_t len, dmell_argv_t* out_argv )
{
    if( cmd == NULL || out_argv == NULL || len == 0 )
    {
        DMOD_LOG_ERROR("Invalid arguments to dmell_parse_command: %p, %p, %zu\n", cmd, out_argv, len);
        return -EINVAL;
    }

    int result = 0;
    const char* end_ptr = cmd + len;
    const char* ptr = cmd;
    while( ptr < end_ptr )
    {
        ptr = skip_whitespaces( ptr, end_ptr );
        const char* arg = ptr;
        const char* next_arg = get_next_arg(arg, end_ptr);
        result = add_arg( out_argv, arg, next_arg, end_ptr );
        if( result < 0 )
        {
            DMOD_LOG_ERROR("Failed to add argument in dmell_parse_command\n");
            free_dmell_argv( out_argv );
            return result;
        }
        ptr = (next_arg != NULL) ? next_arg : end_ptr;
    }

    return 0;
}