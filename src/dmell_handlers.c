#include <errno.h>
#include <string.h>
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
    char* ptr = eval;
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

int dmell_handler_unset( int argc, char** argv )
{
    return -1;
}

int dmell_handler_export( int argc, char** argv )
{
    return -1;
}

int dmell_handler_cd( int argc, char** argv )
{
    return -1;
}

int dmell_handler_exit( int argc, char** argv )
{
    return -1;
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
    dmell_register_command_handler( "exit", dmell_handler_exit );
    return 0;
}