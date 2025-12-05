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
    if(strcmp(command, "export") == 0)
    {
        int result = Dmod_SetEnv( var_name, var_value, 1 );
        if( result != 0 )
        {
            DMOD_LOG_ERROR("Failed to set environment variable in dmell_handler_export: %s=%s\n", var_name, var_value);
            return result;
        }
    }
    else 
    {
        g_dmell_global_script_ctx.variables = dmell_set_variable( g_dmell_global_script_ctx.variables, var_name, var_value );
    }
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
 * @brief Handler for the 'setloglevel' command.
 * 
 * @param argc Number of arguments
 * @param argv Array of argument strings
 * @return int Exit code
 */
int dmell_handler_setloglevel( int argc, char** argv )
{
    if( argc < 2 )
    {
        DMOD_LOG_ERROR("Usage: setloglevel <verbose|info|warning|error>\n");
        return -EINVAL;
    }

    const char* level = argv[1];
    Dmod_LogLevel_t log_level;

    if( strcmp( level, "verbose" ) == 0 )
    {
        log_level = Dmod_LogLevel_Verbose;
    }
    else if( strcmp( level, "info" ) == 0 )
    {
        log_level = Dmod_LogLevel_Info;
    }
    else if( strcmp( level, "warning" ) == 0 )
    {
        log_level = Dmod_LogLevel_Warn;
    }
    else if( strcmp( level, "error" ) == 0 )
    {
        log_level = Dmod_LogLevel_Error;
    }
    else
    {
        DMOD_LOG_ERROR("Invalid log level: %s. Use verbose, info, warning, or error.\n", level);
        return -EINVAL;
    }

    Dmod_SetLogLevel( log_level );
    return 0;
}

/**
 * @brief Helper function to read the shebang line and extract the interpreter.
 * 
 * @param file_name Name of the script file
 * @param buffer Buffer to store the interpreter path
 * @param buffer_size Size of the buffer
 * @return bool True if shebang found and interpreter extracted, false otherwise
 */
static bool get_shebang_interpreter(const char* file_name, char* buffer, size_t buffer_size)
{
    void* file = Dmod_FileOpen(file_name, "r");
    if(file == NULL)
    {
        return false;
    }

    char mark[3] = {0};
    size_t read_bytes = Dmod_FileRead(buffer, 1, 2, file);
    if(read_bytes < 2 || mark[0] != '#' || mark[1] != '!')
    {
        Dmod_FileClose(file);
        return false;
    }

    read_bytes = Dmod_FileRead(buffer, 1, buffer_size - 1, file);
    Dmod_FileClose(file);
    buffer[read_bytes] = '\0';
    return true;
}

/**
 * @brief Helper function to check if a file is a dmell script based on its extension.
 * 
 * @param file_name Name of the file
 * @return bool True if it is a dmell script, false otherwise
 */
static bool is_dmell_script(const char* file_name)
{
    size_t len = strlen(file_name);
    return (len > 4 && strcmp(&file_name[len - 4], ".dme") == 0);
}

/**
 * @brief Helper function to run a shebang interpreter with the script file.
 * 
 * @param interpreter Path to the interpreter
 * @param script_file Path to the script file
 * @param argc Number of arguments
 * @param argv Array of argument strings
 * @return int Exit code
 */
static int run_shebang( char* interpreter, char* script_file, int argc, char** argv )
{
    int new_argc = argc + 2;
    char** new_argv = Dmod_Malloc( sizeof(char*) * (new_argc + 2) );
    if( new_argv == NULL )
    {
        DMOD_LOG_ERROR("Memory allocation failed in run_shebang for new_argv\n");
        return -ENOMEM;
    }

    if(strcmp(interpreter, script_file) == 0)
    {
        DMOD_LOG_ERROR("Circular dependency detected: Interpreter and script file cannot be the same: %s\n", interpreter);
        Dmod_Free( new_argv );
        return -EINVAL;
    }

    new_argv[0] = interpreter;
    new_argv[1] = script_file;
    for( int i = 1; i < argc; i++ )
    {
        new_argv[i + 1] = argv[i];
    }

    int result = dmell_run_command( interpreter, new_argc, new_argv );
    Dmod_Free( new_argv );
    return result;
}

/**
 * @brief Default handler for unknown commands.
 * 
 * @param argc Number of arguments
 * @param argv Array of argument strings
 * @return int Exit code
 */
int dmell_handler_default( int argc, char** argv )
{
    if( argc < 1 )
    {
        DMOD_LOG_ERROR("No command provided to default handler\n");
        return -EINVAL;
    }

    // check if it is setting an environment variable
    if(strchr(argv[0], '=') != NULL)
    {
        return dmell_handler_set( argc, argv );
    }
    else
    {
        // check if it is a file execution
        char* file_name = argv[0];
        if(Dmod_FileAvailable(file_name))
        {
            char interpreter[256] = {0};
            if(get_shebang_interpreter(file_name, interpreter, sizeof(interpreter)))
            {
                return run_shebang(interpreter, file_name, argc, argv);
            }
            else if(is_dmell_script(file_name))
            {
                return dmell_run_script_file(file_name, argc, argv);
            }
            else
            {
                return Dmod_RunModule( file_name, argc, argv );
            }
        }
        else 
        {
            return Dmod_RunModule( file_name, argc, argv );
        }
    }
}

/**
 * @brief Registers built-in command handlers.
 * 
 * @return int Exit code
 */
int dmell_register_handlers( void )
{
    // Set default log level to warning
    Dmod_SetLogLevel( Dmod_LogLevel_Warn );

    dmell_register_command_handler( "echo", dmell_handler_echo );
    dmell_register_command_handler( "set", dmell_handler_set );
    dmell_register_command_handler( "unset", dmell_handler_unset );
    dmell_register_command_handler( "export", dmell_handler_set );
    dmell_register_command_handler( "cd", dmell_handler_cd );
    dmell_register_command_handler( "pwd", dmell_handler_pwd );
    dmell_register_command_handler( "exit", dmell_handler_exit );
    dmell_register_command_handler( "setloglevel", dmell_handler_setloglevel );

    dmell_set_default_handler( dmell_handler_default );
    return 0;
}
