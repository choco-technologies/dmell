#include <errno.h>
#include <string.h>
#include "dmell_script.h"
#include "dmod.h"

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

/**
 * @brief Executes a line of commands in the context of a script, with variable expansion.
 * 
 * @param ctx Script execution context
 * @param line Command line string
 * @param len Length of the command line string
 * @return  int Exit code of the last executed command, or negative value on error
 */
int dmell_run_script_line( dmell_script_ctx_t* ctx, const char* line, size_t len )
{
    if( ctx == NULL || line == NULL || len == 0 )
    {
        DMOD_LOG_ERROR("Invalid arguments to dmell_run_script_line: %p, %p, %zu\n", ctx, line, len);
        return -EINVAL;
    }
    const char* comment_start = find_comment_start( line, line + len );
    size_t effective_len = comment_start - line;
    if( effective_len == 0 )
    {
        // Line is empty or a comment
        return 0;
    }
    int required_size = dmell_expand_variables( ctx->variables, line, effective_len, NULL, 0 );
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

    if(dmell_expand_variables( ctx->variables, line, effective_len, expanded_line, required_size + 1 ) < 0 )
    {
        DMOD_LOG_ERROR("Failed to expand variables in dmell_run_script_line\n");
        Dmod_Free( expanded_line );
        return -EINVAL;
    }

    int exit_code = dmell_run_line( expanded_line, effective_len );
    Dmod_Free( expanded_line );
    
    char code_str[12];
    Dmod_SnPrintf( code_str, sizeof(code_str), "%d", exit_code );
    ctx->variables      = dmell_set_variable( ctx->variables, "?", code_str );
    ctx->last_exit_code = exit_code;

    return exit_code;
}

/**
 * @brief Executes a script file with given arguments.
 * 
 * @param file_path Path to the script file
 * @param argc Number of arguments
 * @param argv Array of argument strings
 * @return int Exit code of the script execution, or negative value on error
 */
int dmell_run_script_file(const char* file_path, int argc, char** argv)
{
    if( file_path == NULL )
    {
        DMOD_LOG_ERROR("Invalid file path in dmell_run_script_file: %p\n", file_path);
        return -EINVAL;
    }

    void* file = Dmod_FileOpen( file_path, "r" );
    if( file == NULL )
    {
        DMOD_LOG_ERROR("Failed to open script file: %s\n", file_path);
        return -ENOENT;
    }

    dmell_script_ctx_t script_ctx = {
        .last_exit_code = 0,
        .variables      = NULL
    };

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
        int exit_code = dmell_run_script_line( &script_ctx, line, line_len );
        if( exit_code < 0 )
        {
            DMOD_LOG_ERROR("Error executing line %d in script file %s\n", line_number, file_path);
            Dmod_Free( line );
            Dmod_FileClose( file );
            return exit_code;
        }
    }

    return 0;
}