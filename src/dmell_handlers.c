#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include <dirent.h>
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
 * @brief Helper function to check if a path is a directory.
 * 
 * @param path Path to check
 * @return bool True if path is a directory, false otherwise
 */
static bool is_dir(const char* path)
{
    void* dir = Dmod_OpenDir(path);
    if(dir != NULL)
    {
        Dmod_CloseDir(dir);
        return true;
    }
    return false;
}

/**
 * @brief Helper function to get the filename from a given path.
 * 
 * @param path Full file path
 * @return const char* Filename extracted from the path
 */
static const char* get_filename_from_path(const char* path)
{
    const char* last_slash = strrchr(path, '/');
    if(last_slash != NULL)
    {
        return last_slash + 1;
    }
    return path;
}

/**
 * @brief Handler for the 'cp' command.
 * 
 * @param argc Number of arguments
 * @param argv Array of argument strings
 * @return int Exit code
 */
int dmell_handler_cp( int argc, char** argv )
{
    if( argc < 3 )
    {
        DMOD_LOG_ERROR("Usage: cp <source> <destination>\n");
        return -EINVAL;
    }

    const char* source = argv[1];
    const char* destination = argv[2];
    bool dest_is_dir = is_dir(destination);
    bool alloced_dest_path = false;
    if( source == NULL || destination == NULL )
    {
        DMOD_LOG_ERROR("Invalid source or destination in cp command\n");
        return -EINVAL;
    }

    if(dest_is_dir)
    {
        const char* filename = get_filename_from_path(source);
        size_t dest_path_len = strlen(destination) + 1 + strlen(filename) + 1;
        char* dest_path = Dmod_Malloc(dest_path_len);
        if(dest_path == NULL)
        {
            DMOD_LOG_ERROR("Memory allocation failed in cp command\n");
            return -ENOMEM;
        }
        Dmod_SnPrintf(dest_path, dest_path_len, "%s/%s", destination, filename);
        destination = dest_path;
        alloced_dest_path = true;
    }

    void* src_file = Dmod_FileOpen(source, "rb");
    if( src_file == NULL )
    {
        DMOD_LOG_ERROR("Failed to open source file '%s'\n", source);
        return -1;
    }

    void* dest_file = Dmod_FileOpen(destination, "wb");
    if( dest_file == NULL )
    {
        DMOD_LOG_ERROR("Failed to open destination file '%s'\n", destination);
        Dmod_FileClose(src_file);
        return -1;
    }

    char buffer[4096];
    size_t bytes_read;
    while( (bytes_read = Dmod_FileRead(buffer, 1, sizeof(buffer), src_file)) > 0 )
    {
        size_t bytes_written = Dmod_FileWrite(buffer, 1, bytes_read, dest_file);
        if( bytes_written < bytes_read )
        {
            DMOD_LOG_ERROR("Failed to write to destination file '%s'\n", destination);
            Dmod_FileClose(src_file);
            Dmod_FileClose(dest_file);
            return -1;
        }
    }

    Dmod_FileClose(src_file);
    Dmod_FileClose(dest_file);
    if(alloced_dest_path)
    {
        Dmod_Free((void*)destination);
    }
    return 0;
}

/**
 * @brief Handler for the 'ls' command.
 * 
 * @param argc Number of arguments
 * @param argv Array of argument strings
 * @return int Exit code
 */
int dmell_handler_ls( int argc, char** argv )
{
    const char* path = ".";
    int show_hidden = 0;
    int long_format = 0;
    
    // Parse arguments
    for( int i = 1; i < argc; i++ )
    {
        if( argv[i][0] == '-' )
        {
            // Handle flags
            for( int j = 1; argv[i][j] != '\0'; j++ )
            {
                switch( argv[i][j] )
                {
                    case 'a':
                        show_hidden = 1;
                        break;
                    case 'l':
                        long_format = 1;
                        break;
                    default:
                        DMOD_LOG_ERROR("Unknown option: -%c\n", argv[i][j]);
                        return -EINVAL;
                }
            }
        }
        else
        {
            // Path argument
            path = argv[i];
        }
    }
    
    void* dir = Dmod_OpenDir(path);
    if( dir == NULL )
    {
        DMOD_LOG_ERROR("Failed to open directory '%s'\n", path);
        return -1;
    }
    
    const char* entry;
    while( (entry = Dmod_ReadDir(dir)) != NULL )
    {
        // Skip hidden files unless -a flag is used
        if( !show_hidden && entry[0] == '.' )
        {
            continue;
        }
        
        if( long_format )
        {
            // Simple long format - just show name (could be extended with file stats)
            Dmod_Printf("%-20s\n", entry);
        }
        else
        {
            Dmod_Printf("%s  ", entry);
        }
    }
    
    if( !long_format )
    {
        Dmod_Printf("\n");
    }
    
    Dmod_CloseDir(dir);
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
    dmell_register_command_handler( "echo", dmell_handler_echo );
    dmell_register_command_handler( "set", dmell_handler_set );
    dmell_register_command_handler( "unset", dmell_handler_unset );
    dmell_register_command_handler( "export", dmell_handler_set );
    dmell_register_command_handler( "cd", dmell_handler_cd );
    dmell_register_command_handler( "pwd", dmell_handler_pwd );
    dmell_register_command_handler( "ls", dmell_handler_ls );
    dmell_register_command_handler( "cp", dmell_handler_cp );
    dmell_register_command_handler( "exit", dmell_handler_exit );

    dmell_set_default_handler( dmell_handler_default );
    return 0;
}