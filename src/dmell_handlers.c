#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include <dmod.h>
#include "dmell_handlers.h"
#include "dmell.h"

/**
 * @brief Handler for the 'echo' command.
 * 
 * @param argc Number of arguments
 * @param argv Array of argument strings
 * @return int Exit code
 */
int dmell_handler_echo( int argc, char** argv )
{
    for( int i = 1; i < argc; i++ )
    {
        Dmod_Printf("%s", argv[i]);
        if( i < argc - 1 )
        {
            Dmod_Printf(" ");
        }
    }
    Dmod_Printf("\n");
    return 0;
}

/**
 * @brief Handler for the 'set' command.
 * 
 * @param argc Number of arguments
 * @param argv Array of argument strings
 * @return int Exit code
 */
int dmell_handler_set( int argc, char** argv )
{
    if( argc < 1 || argv == NULL )
    {
        DMOD_LOG_ERROR("Invalid arguments to dmell_handler_set: %d, %p\n", argc, argv);
        return -EINVAL;
    }

    const char* command = argv[0];
    const char* eval = NULL;
    if(strcmp(command, "set") == 0 || strcmp(command, "export") == 0)
    {
        if(argc < 2)
        {
            DMOD_LOG_ERROR("Missing evaluation for '%s'\n", command);
            return -EINVAL;
        }
        eval = argv[1];
    }
    else 
    {
        eval = argv[0];
    }
    const char* ptr = eval;
    while(ptr != NULL & *ptr != '=' && *ptr != '\0')
    {
        ptr++;
    }
    if(*ptr != '=')
    {
        DMOD_LOG_ERROR("Invalid variable assignment in dmell_handler_set: %s\n", eval);
        return -EINVAL;
    }

    size_t name_len = ptr - eval;
    ptr++;
    if(name_len <= 0)
    {
        DMOD_LOG_ERROR("Invalid variable name in dmell_handler_set: %s\n", eval);
        return -EINVAL;
    }
    char var_name[name_len + 1];
    strncpy(var_name, eval, name_len);
    var_name[name_len] = '\0';
    const char* var_value = ptr;
    g_dmell_global_script_ctx.variables = dmell_set_variable( g_dmell_global_script_ctx.variables, var_name, var_value );
    return 0;
}

/**
 * @brief Handler for the 'unset' command.
 * 
 * @param argc Number of arguments
 * @param argv Array of argument strings
 * @return int Exit code
 */
int dmell_handler_unset( int argc, char** argv )
{
    if( argc < 2 )
    {
        DMOD_LOG_ERROR("Missing variable name for 'unset' command\n");
        return -EINVAL;
    }

    for( int i = 1; i < argc; i++ )
    {
        const char* var_name = argv[i];
        if( var_name == NULL || strlen(var_name) == 0 )
        {
            DMOD_LOG_ERROR("Invalid variable name in unset: %s\n", var_name ? var_name : "(null)");
            continue;
        }
        g_dmell_global_script_ctx.variables = dmell_remove_variable( g_dmell_global_script_ctx.variables, var_name );
    }
    return 0;
}

/**
 * @brief Handler for the 'cd' command.
 * 
 * @param argc Number of arguments
 * @param argv Array of argument strings
 * @return int Exit code
 */
int dmell_handler_cd( int argc, char** argv )
{
    const char* path = NULL;
    
    if( argc < 2 )
    {
        // No argument provided, change to home directory
        path = Dmod_GetEnv("HOME");
        if( path == NULL )
        {
            DMOD_LOG_ERROR("HOME environment variable not set\n");
            return -EINVAL;
        }
    }
    else
    {
        path = argv[1];
    }

    if( path == NULL || strlen(path) == 0 )
    {
        DMOD_LOG_ERROR("Invalid directory path\n");
        return -EINVAL;
    }

    int result = Dmod_ChDir(path);
    if( result != 0 )
    {
        DMOD_LOG_ERROR("Failed to change directory to '%s': %d\n", path, result);
        return result;
    }

    return 0;
}

/**
 * @brief Handler for the 'pwd' command.
 * 
 * @param argc Number of arguments
 * @param argv Array of argument strings
 * @return int Exit code
 */
int dmell_handler_pwd( int argc, char** argv )
{
    char cwd[1024];  // Use a reasonable buffer size for path
    
    if( Dmod_GetCwd(cwd, sizeof(cwd)) == NULL )
    {
        DMOD_LOG_ERROR("Failed to get current working directory\n");
        return -1;
    }
    
    Dmod_Printf("%s\n", cwd);
    return 0;
}

/**
 * @brief Handler for the 'exit' command.
 * 
 * @param argc Number of arguments
 * @param argv Array of argument strings
 * @return int Exit code
 */
int dmell_handler_exit( int argc, char** argv )
{
    int exit_code = 0;
    
    if( argc >= 2 )
    {
        // Parse exit code from argument
        long code = 0;
        const char* str = argv[1];
        char* endptr = NULL;

        // Simple manual conversion from string to integer
        while (*str != '\0' && *str >= '0' && *str <= '9') {
            code = code * 10 + (*str - '0');
            str++;
        }
        endptr = (char*)str;
        
        if( *endptr != '\0' || code < INT_MIN || code > INT_MAX )
        {
            DMOD_LOG_ERROR("Invalid exit code: %s\n", argv[1]);
            exit_code = 2;
        }
        else
        {
            exit_code = (int)code;
        }
    }
    else
    {
        // Use last command's exit code if no argument provided
        exit_code = g_dmell_global_script_ctx.last_exit_code;
    }
    
    // Signal exit by returning a special value (negative for error handling)
    return exit_code == 0 ? -255 : -exit_code;
}

/**
 * @brief Registers built-in command handlers.
 * 
 * @return int Exit code
 */
int dmell_register_handlers( void )
{
    dmell_register_command_handler( "echo", dmell_handler_echo );
    dmell_register_command_handler( "set", dmell_handler_set );
    dmell_register_command_handler( "unset", dmell_handler_unset );
    dmell_register_command_handler( "export", dmell_handler_set );
    dmell_register_command_handler( "cd", dmell_handler_cd );
    dmell_register_command_handler( "pwd", dmell_handler_pwd );
    dmell_register_command_handler( "exit", dmell_handler_exit );
    return 0;
}