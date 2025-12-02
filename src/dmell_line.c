#include <stdbool.h>
#include <errno.h>
#include <dmod.h>
#include <string.h>
#include "dmell_cmd.h"
#include "dmell_line.h"
#include "dmell_hlp.h"

/**
 * @brief Helper function to check if the current position is an 'OR' separator (||).
 * 
 * @param str Current position in the command string
 * @param end_ptr Pointer to the end of the command string
 * @return true If it is an 'OR' separator
 * @return false Otherwise
 */
static bool is_or_separator( const char* str, const char* end_ptr )
{
    return ( str + 1 < end_ptr ) && ( str[0] == '|' ) && ( str[1] == '|' );
}

/**
 * @brief Helper function to check if the current position is an 'AND' separator (&&).
 * 
 * @param str Current position in the command string
 * @param end_ptr Pointer to the end of the command string
 * @return true If it is an 'AND' separator
 * @return false Otherwise
 */
static bool is_and_separator( const char* str, const char* end_ptr )
{
    return ( str + 1 < end_ptr ) && ( str[0] == '&' ) && ( str[1] == '&' );
}

/**
 * @brief Helper function to check if the current position is a sequence separator (; or \n).
 * 
 * @param str Current position in the command string
 * @param end_ptr Pointer to the end of the command string
 * @return true If it is a sequence separator
 * @return false Otherwise
 */
static bool is_sequence_separator( const char* str, const char* end_ptr )
{
    return str < end_ptr && ( str[0] == ';' || str[0] == '\n' );
}

/**
 * @brief Helper function to get the type of command separator at the current position.
 * 
 * @param str Current position in the command string
 * @param end_ptr Pointer to the end of the command string
 * @return dmell_line_sep_t Type of command separator
 */
static dmell_line_sep_t get_command_separator( const char* str, const char* end_ptr )
{
    if( is_and_separator( str, end_ptr ) )
    {
        return dmell_line_sep_and;
    }
    else if( is_or_separator( str, end_ptr ) )
    {
        return dmell_line_sep_or;
    }
    else if( is_sequence_separator( str, end_ptr ) )
    {
        return dmell_line_sep_seq;
    }
    return dmell_line_sep_none;
}

/**
 * @brief Helper function to skip a command separator in the command string.
 * 
 * @param str Current position in the command string
 * @param end_ptr Pointer to the end of the command string
 * @param sep Type of command separator to skip
 * @return const char* Pointer to the position after the skipped separator
 */
static const char* skip_separator( const char* str, const char* end_ptr, dmell_line_sep_t sep )
{
    size_t sep_len[dmell_line_sep_max] = {
        [dmell_line_sep_none] = 0,
        [dmell_line_sep_and]  = 2,
        [dmell_line_sep_or]   = 2,
        [dmell_line_sep_seq]  = 1
    };

    const char* ptr = str + sep_len[sep];
    if( ptr > end_ptr )
    {
        ptr = end_ptr;
    }
    return ptr;
}

/**
 * @brief Helper function to find the next command separator in the command string.
 * 
 * @param str Current position in the command string
 * @param end_ptr Pointer to the end of the command string
 * @param out_sep Output parameter to hold the type of found separator
 * @return const char* Pointer to the position of the next command separator, or end_ptr if none found
 */
static const char* find_next_command_separator(const char* str, const char* end_ptr, dmell_line_sep_t* out_sep)
{
    const char* ptr = str;
    while( ptr < end_ptr )
    {
        dmell_line_sep_t sep = get_command_separator( ptr, end_ptr );
        if( sep != dmell_line_sep_none )
        {            
            if( out_sep != NULL )
            {
                *out_sep = sep;
            }
            return ptr;
        }
        ptr++;
    }
    if( out_sep != NULL )
    {
        *out_sep = dmell_line_sep_none;
    }
    return end_ptr;
}

/**
 * @brief Helper function to combine exit codes based on the command separator.
 * 
 * @param last_exit_code Exit code of the last executed command
 * @param current_exit_code Exit code of the current executed command
 * @param sep Type of command separator
 * @return int Combined exit code
 */
static int join_results(int last_exit_code, int current_exit_code, dmell_line_sep_t sep)
{
    switch( sep )
    {
        case dmell_line_sep_and:
            return (last_exit_code == 0) ? current_exit_code : last_exit_code;
        case dmell_line_sep_or:
            return (last_exit_code != 0) ? current_exit_code : last_exit_code;
        case dmell_line_sep_seq:
            return current_exit_code;
        case dmell_line_sep_none:
        default:
            return current_exit_code;
    }
}

/**
 * @brief Helper function to determine if the next command should be executed based on the last exit code and separator.
 * 
 * @param last_exit_code Exit code of the last executed command
 * @param sep Type of command separator
 * @return true If the next command should be executed
 * @return false Otherwise
 */
static bool should_execute_command(int last_exit_code, dmell_line_sep_t sep)
{
    switch( sep )
    {
        case dmell_line_sep_and:
            return (last_exit_code == 0);
        case dmell_line_sep_or:
            return (last_exit_code != 0);
        case dmell_line_sep_seq:
        case dmell_line_sep_none:
        default:
            return true;
    }
}

/**
 * @brief Helper function to calculate the total length of a command line from arguments.
 * 
 * @param argc Number of arguments
 * @param argv Array of argument strings
 * @return size_t Total length of the command line
 */
static size_t calculate_line_length(int argc, char** argv)
{
    size_t total_length = 0;
    for( int i = 0; i < argc; i++ )
    {
        total_length += strlen( argv[i] );
        if( i < argc - 1 )
        {
            total_length += 1; // for space
        }
    }
    return total_length;
}

/**
 * @brief Helper function to convert arguments to a command line string.
 * 
 * @param argc Number of arguments
 * @param argv Array of argument strings
 * @param out_len Output parameter to hold the length of the command line string
 * @return char* Command line string, or NULL on failure
 */
static char* to_line(int argc, char** argv, size_t* out_len)
{
    size_t line_length = calculate_line_length( argc, argv );
    char* line = Dmod_Malloc( line_length + 1 );
    if( line == NULL )
    {
        DMOD_LOG_ERROR("Memory allocation failed in to_line\n");
        return NULL;
    }

    char* ptr = line;
    for( int i = 0; i < argc; i++ )
    {
        size_t arg_len = strlen( argv[i] );
        memcpy( ptr, argv[i], arg_len );
        ptr += arg_len;
        if( i < argc - 1 )
        {
            *ptr = ' ';
            ptr++;
        }
    }
    *ptr = '\0';

    if( out_len != NULL )
    {
        *out_len = line_length;
    }

    return line;
}

/**
 * @brief Executes a line of commands with proper handling of separators.
 * 
 * @param line Command line string
 * @param len Length of the command line string
 * @return int Exit code of the last executed command, or negative value on error
 */
int dmell_run_line(const char* line, size_t len)
{
    if(line == NULL || len == 0)
    {
        DMOD_LOG_ERROR("Invalid arguments to dmell_run_line: %p, %zu\n", line, len);
        return -EINVAL;
    }

    const char* end_ptr = line + len;
    const char* ptr = line;
    int last_exit_code = 0;
    int result = 0;
    dmell_line_sep_t prev_sep = dmell_line_sep_none;
    while( ptr < end_ptr && *ptr != '\0' )
    {
        // Find the next command separator
        dmell_line_sep_t sep = dmell_line_sep_none;
        const char* sep_ptr = find_next_command_separator( ptr, end_ptr, &sep );

        // Check if we should execute the current command - lazy evaluation
        // Use the previous separator to decide if the current command should run
        if( should_execute_command( last_exit_code, prev_sep ) )
        {
            // Determine the length of the current command
            size_t cmd_len = sep_ptr - ptr;
            if( cmd_len > 0 )
            {
                // Execute the current command
                int exit_code = dmell_run_command_string( ptr, cmd_len );
                result = join_results( last_exit_code, exit_code, prev_sep );
                last_exit_code = exit_code;
            }
        }

        // Move to the next command
        ptr = skip_separator( sep_ptr, end_ptr, sep );
        prev_sep = sep;
    }

    return result;
}

/**
 * @brief Executes a line of commands from argument array.
 * 
 * @param argc Number of arguments
 * @param argv Array of argument strings
 * @return int Exit code of the last executed command, or negative value on error
 */
int dmell_run_args_line(int argc, char** argv)
{
    if( argc <= 0 || argv == NULL )
    {
        DMOD_LOG_ERROR("Invalid arguments to dmell_run_args_line: %d, %p\n", argc, argv);
        return -EINVAL;
    }

    size_t line_length = 0;
    char* line = to_line( argc, argv, &line_length );
    if( line == NULL )
    {
        DMOD_LOG_ERROR("Failed to convert arguments to line in dmell_run_args_line\n");
        return -ENOMEM;
    }

    int result = dmell_run_line( line, line_length );
    Dmod_Free( line );
    return result;
}