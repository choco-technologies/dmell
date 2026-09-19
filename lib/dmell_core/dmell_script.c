#include <errno.h>
#include <string.h>
#include "dmell_script.h"
#include "dmell_vars.h"
#include "dmod.h"
#include "dmell_hlp.h"
#include "dmell_cmd.h"

/**
 * @brief Helper function to find the start of a comment in the script line.
 *
 * @param str Current position in the script line
 * @param end_ptr Pointer to the end of the script line
 * @return const char* Pointer to the position of the comment start, or end_ptr if none found
 */
static const char* find_comment_start( const char* str, const char* end_ptr )
{
    const char* ptr = str;
    while( ptr < end_ptr )
    {
        if( *ptr == '#' )
        {
            return ptr;
        }
        ptr++;
    }
    return end_ptr;
}

int dmell_run_script_line( dmell_ctx_t* ctx, const char* line, size_t len )
{
    if(len == 0)
    {
        return 0;
    }
    if( ctx == NULL || line == NULL )
    {
        DMOD_LOG_ERROR("Invalid arguments to dmell_run_script_line: %p, %p, %zu\n", ctx, line, len);
        return -EINVAL;
    }
    const char* end_ptr = line + len;
    line = dmell_skip_whitespaces( line, end_ptr );
    if( line >= end_ptr )
    {
        // Line is empty or whitespace only
        return 0;
    }
    const char* comment_start = find_comment_start( line, end_ptr );
    size_t effective_len = comment_start - line;
    if( effective_len == 0 )
    {
        // Line is empty or a comment
        return 0;
    }
    dmell_var_t* variables = (dmell_var_t*)ctx->variables;
    int required_size = dmell_expand_variables( variables, line, effective_len, NULL, 0 );
    if( required_size < 0 )
    {
        DMOD_LOG_ERROR("Failed to calculate required size for variable expansion in dmell_run_script_line\n");
        return required_size;
    }

    char* expanded_line = Dmod_Malloc( required_size + 1 );
    if( expanded_line == NULL )
    {
        DMOD_LOG_ERROR("Memory allocation failed in dmell_run_script_line for expanded line\n");
        return -ENOMEM;
    }

    if(dmell_expand_variables( variables, line, effective_len, expanded_line, required_size + 1 ) < 0 )
    {
        DMOD_LOG_ERROR("Failed to expand variables in dmell_run_script_line\n");
        Dmod_Free( expanded_line );
        return -EINVAL;
    }
    // required_size, not effective_len: the latter measures the line *before*
    // expansion. Handing it to dmell_run_line() as the expanded buffer's length
    // truncates the command whenever a variable expanded to something longer
    // than its name, and reads past the end of the buffer whenever it expanded
    // to something shorter.
    int result = dmell_run_line( ctx, expanded_line, (size_t)required_size );
    Dmod_Free( expanded_line );

    // What "$?" and the next line's && / || must see is the *status*, whether
    // it came back on its own or wrapped in an exit request.
    int exit_code = DMELL_IS_EXIT_REQUEST( result ) ? DMELL_EXIT_REQUEST_STATUS( result ) : result;

    char code_str[12];
    Dmod_SnPrintf( code_str, sizeof(code_str), "%d", exit_code );
    ctx->variables      = (void*)dmell_set_variable( (dmell_var_t*)ctx->variables, "?", code_str );
    ctx->last_exit_code = exit_code;

    // The request itself keeps travelling, so whoever is running the script
    // knows to stop rather than carry on to the next line.
    return result;
}

int dmell_run_script_file( dmell_ctx_t* ctx, const char* file_path, int argc, char** argv)
{
    (void)argc;
    (void)argv;

    if( ctx == NULL || file_path == NULL )
    {
        DMOD_LOG_ERROR("Invalid arguments to dmell_run_script_file: %p, %p\n", ctx, file_path);
        return -EINVAL;
    }

    void* file = Dmod_FileOpen( file_path, "r" );
    if( file == NULL )
    {
        DMOD_LOG_ERROR("Failed to open script file: %s\n", file_path);
        return -ENOENT;
    }

    size_t line_len = 0;
    char* line = Dmod_Malloc( DMELL_MAX_SCRIPT_LINE_LENGTH );
    if (line == NULL )
    {
        DMOD_LOG_ERROR("Memory allocation failed in dmell_run_script_file for line buffer\n");
        Dmod_FileClose( file );
        return -ENOMEM;
    }

    int line_number = 0;
    while( Dmod_FileReadLine( line, DMELL_MAX_SCRIPT_LINE_LENGTH, file ) != NULL )
    {
        line_number++;
        line_len = strlen( line );
        int result = dmell_run_script_line( ctx, line, line_len );
        if( DMELL_IS_EXIT_REQUEST( result ) )
        {
            // The script asked to stop. That is how it is supposed to end, so
            // it leaves through the same door as running off the last line.
            Dmod_Free( line );
            Dmod_FileClose( file );
            return DMELL_EXIT_REQUEST_STATUS( result );
        }
        if( result < 0 )
        {
            DMOD_LOG_ERROR("Error executing line %d in script file %s\n", line_number, file_path);
            Dmod_Free( line );
            Dmod_FileClose( file );
            return result;
        }
    }

    Dmod_Free( line );
    Dmod_FileClose( file );
    return 0;
}
