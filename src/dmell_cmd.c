#include "dmell_hlp.h"
#include "dmell_cmd.h"
#include "dmell_redirect.h"
#include <dmod.h>
#include <string.h>
#include <errno.h>

/**
 * @brief Global array of registered commands.
 */
dmell_cmd_t* g_registered_commands = NULL;
/**
 * @brief Count of registered commands.
 */
size_t g_registered_command_count = 0;

/**
 * @brief Default command handler function pointer.
 */
dmell_cmd_handler_t g_default_command_handler = NULL;

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
    memset( new_arg, 0, len + 1 );
    if ((arg[0] == '"' && arg[len - 1] == '"') || (arg[0] == '\'' && arg[len - 1] == '\'')) {
        memcpy(new_arg, arg + 1, len - 2);
    } else {
        memcpy(new_arg, arg, len);
    }
    return new_arg;
}

/**
 * @brief Helper function to append a parsed redirection operator to a dmell_argv_t.
 *
 * @param argv Pointer to the dmell_argv_t structure
 * @param redirect Redirection entry to append (its path ownership is transferred)
 * @return int 0 on success, negative value on error
 */
static int add_redirect( dmell_argv_t* argv, const dmell_redirect_t* redirect )
{
    dmell_redirect_t* new_redirects = Dmod_Realloc( argv->redirects, sizeof(dmell_redirect_t) * (argv->redirect_count + 1) );
    if( new_redirects == NULL )
    {
        DMOD_LOG_ERROR("Memory allocation failed in add_redirect\n");
        return -ENOMEM;
    }

    argv->redirects = new_redirects;
    argv->redirects[argv->redirect_count] = *redirect;
    argv->redirect_count += 1;
    return 0;
}

/**
 * @brief Helper function to remove a run of tokens from argv, freeing them.
 *
 * @param argv Pointer to the dmell_argv_t structure
 * @param index Index of the first token to remove
 * @param count Number of consecutive tokens to remove
 */
static void remove_tokens( dmell_argv_t* argv, int index, int count )
{
    /* Free the tokens actually being removed first - shifting the tail left
     * over them would silently leak these pointers, and freeing the (now
     * duplicated) tail slots afterwards instead would free memory that's
     * still referenced by the shifted-in positions. */
    for( int j = index; j < index + count; j++ )
    {
        Dmod_Free( argv->argv[j] );
    }

    for( int j = index; j < argv->argc - count; j++ )
    {
        argv->argv[j] = argv->argv[j + count];
    }

    argv->argc -= count;
}

/**
 * @brief Scan already-tokenized arguments for redirection operators, stripping
 *        them out of argv and recording them on out_argv->redirects in the
 *        order they were encountered (order matters for 'N>&M' duplication).
 *
 * @param out_argv Structure whose argv array is scanned and mutated in place
 * @return int 0 on success, negative value on error
 */
static int extract_redirects( dmell_argv_t* out_argv )
{
    for( int i = 0; i < out_argv->argc; )
    {
        const char* token = out_argv->argv[i];
        dmell_redirect_match_t match;

        if( !dmell_redirect_match_token( token, &match ) )
        {
            i++;
            continue;
        }

        int tokens_to_remove = 1;
        const char* target = match.attached_target;

        if( !match.is_dup && match.needs_target )
        {
            if( i + 1 >= out_argv->argc || out_argv->argv[i + 1][0] == '\0' )
            {
                DMOD_LOG_ERROR("Missing redirect target after '%s'\n", token);
                return -EINVAL;
            }
            target = out_argv->argv[i + 1];
            tokens_to_remove = 2;
        }

        for( int op = 0; op < match.op_count; op++ )
        {
            dmell_redirect_t redirect = {0};
            redirect.stream     = match.stream[op];
            redirect.is_dup     = match.is_dup;
            redirect.dup_source = match.dup_source;
            redirect.append     = match.append;

            if( !match.is_dup )
            {
                redirect.path = Dmod_StrDup( target );
                if( redirect.path == NULL )
                {
                    DMOD_LOG_ERROR("Memory allocation failed for redirect target\n");
                    return -ENOMEM;
                }
            }

            int result = add_redirect( out_argv, &redirect );
            if( result < 0 )
            {
                Dmod_Free( redirect.path );
                return result;
            }
        }

        remove_tokens( out_argv, i, tokens_to_remove );
    }

    return 0;
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

    if(argv->program_name == NULL)
    {
        argv->program_name = new_arg;
    }

    return 0;
}

/**
 * @brief Helper function to free the dmell_argv_t structure.
 * 
 * @param argv Pointer to the dmell_argv_t structure to free
 */
void dmell_free_argv( dmell_argv_t* argv )
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

    dmell_redirect_free( argv->redirects, argv->redirect_count );
    argv->redirects = NULL;
    argv->redirect_count = 0;
}

/**
 * @brief Helper function to move a command structure.
 * 
 * @param dest Destination command structure
 * @param src Source command structure
 */
static void move_command( dmell_cmd_t* dest, const dmell_cmd_t* src )
{
    dest->name = src->name;
    dest->handler = src->handler;
}

/**
 * @brief Helper function to copy a command structure.
 * 
 * @param dest Destination command structure
 * @param src Source command structure
 * @return int 0 on success, negative value on error
 */
static int copy_command( dmell_cmd_t* dest, const dmell_cmd_t* src )
{
    dest->name = Dmod_StrDup( src->name );
    if( dest->name == NULL )
    {
        DMOD_LOG_ERROR("Memory allocation failed in Dmod_StrDup for command name\n");
        return -ENOMEM;
    }
    dest->handler = src->handler;
    return 0;
}

/**
 * @brief Helper function to find the index of a command in the registered commands array.
 * 
 * @param command Pointer to the command structure to find
 * @return int Index of the command, or -1 if not found
 */
static int find_command_index( const dmell_cmd_t* command )
{
    for( size_t i = 0; i < g_registered_command_count; i++ )
    {
        if( strcmp( g_registered_commands[i].name, command->name ) == 0 && g_registered_commands[i].handler == command->handler )
        {
            return (int)i;
        }
    }
    return -ENOENT;
}

/**
 * @brief Helper function to free a command structure.
 * 
 * @param command Pointer to the command structure to free
 */
static void free_command( dmell_cmd_t* command )
{
    if( command == NULL )
    {
        return;
    }

    Dmod_Free( (void*)command->name );
    command->name = NULL;
    command->handler = NULL;
}

/**
 * @brief Helper function to shift commands left in the registered commands array.
 * 
 * @param start_index Index to start shifting from
 */
static void shift_commands_left( size_t start_index )
{
    for( size_t i = start_index; i < g_registered_command_count - 1; i++ )
    {
        move_command( &g_registered_commands[i], &g_registered_commands[i + 1] );
    }

    // free the last command slot
    free_command( &g_registered_commands[g_registered_command_count - 1] );
}

/**
 * @brief Helper function to remove a command at a specific index.
 * 
 * @param index Index of the command to remove
 * @return int 0 on success, negative value on error
 */
static int remove_command_at_index( size_t index )
{
    if( index >= g_registered_command_count )
    {
        return -EINVAL;
    }

    shift_commands_left( index );

    dmell_cmd_t* new_commands = NULL;
    if( g_registered_command_count - 1 > 0 )
    {
        new_commands = Dmod_Realloc( g_registered_commands, sizeof(dmell_cmd_t) * (g_registered_command_count - 1) );
        if( new_commands == NULL )
        {
            DMOD_LOG_ERROR("Memory allocation failed in dmell_unregister_command\n");
            return -ENOMEM;
        }
    }
    else
    {
        Dmod_Free( g_registered_commands );
    }

    g_registered_commands = new_commands;
    g_registered_command_count -= 1;
    return 0;
}

/**
 * @brief Sets the default command handler for unrecognized commands.
 * 
 * @param handler Function pointer to the default command handler
 * @return int 0 on success
 */
int dmell_set_default_handler(dmell_cmd_handler_t handler)
{
    g_default_command_handler = handler;
    return 0;
}

/**
 * @brief Registers a command with the dmell module.
 * 
 * @param command Pointer to the command structure to register
 * @return int 0 on success, negative value on error
 */
int dmell_register_command(const dmell_cmd_t* command)
{
    if( command == NULL || command->name == NULL || command->handler == NULL )
    {
        DMOD_LOG_ERROR("Invalid command structure passed to dmell_register_command: %p\n", command);
        return -EINVAL;
    }

    dmell_cmd_t* new_commands = Dmod_Realloc( g_registered_commands, sizeof(dmell_cmd_t) * (g_registered_command_count + 1) );
    if( new_commands == NULL )
    {
        DMOD_LOG_ERROR("Memory allocation failed in dmell_register_command\n");
        return -ENOMEM;
    }

    g_registered_commands = new_commands;
    dmell_cmd_t* new_command = &g_registered_commands[g_registered_command_count];
    int result = copy_command( new_command, command );
    if( result < 0 )
    {
        DMOD_LOG_ERROR("Failed to copy command in dmell_register_command\n");
        return result;
    }
    g_registered_command_count += 1;
    return 0;
}

/**
 * @brief Registers a command handler by command name.
 * 
 * @param command_name Name of the command to register
 * @param handler Function pointer to the command handler
 * @return int 0 on success, negative value on error
 */
int dmell_register_command_handler(const char* command_name, dmell_cmd_handler_t handler)
{
    if( command_name == NULL || handler == NULL )
    {
        DMOD_LOG_ERROR("Invalid arguments to dmell_register_command_handler: %p, %p\n", command_name, handler);
        return -EINVAL;
    }

    dmell_cmd_t command = {
        .name = command_name,
        .handler = handler
    };

    return dmell_register_command( &command );
}

/**
 * @brief Finds a registered command by its name.
 * 
 * @param command_name Name of the command to find
 * @return const dmell_cmd_t* Pointer to the command structure, or NULL if not found
 */
const dmell_cmd_t* dmell_find_command(const char* command_name)
{
    if( command_name == NULL )
    {
        DMOD_LOG_ERROR("Invalid command name passed to dmell_find_command: %p\n", command_name);
        return NULL;
    }

    for( size_t i = 0; i < g_registered_command_count; i++ )
    {
        if( strcmp( g_registered_commands[i].name, command_name ) == 0 )
        {
            return &g_registered_commands[i];
        }
    }

    return NULL;
}

/**
 * @brief Unregisters a command from the dmell module.
 * 
 * @param command Pointer to the command structure to unregister
 * @return int 0 on success, negative value on error
 */
int dmell_unregister_command(const dmell_cmd_t* command)
{
    if( command == NULL || command->name == NULL )
    {
        DMOD_LOG_ERROR("Invalid command structure passed to dmell_unregister_command: %p\n", command);
        return -EINVAL;
    }

    int index = find_command_index( command );
    if( index < 0 )
    {
        DMOD_LOG_ERROR("Command not found in dmell_unregister_command: %s\n", command->name);
        return -ENOENT;
    }

    return remove_command_at_index( (size_t)index );
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
    if( cmd_name == NULL )
    {
        DMOD_LOG_ERROR("Invalid command name passed to dmell_run_command: %p\n", cmd_name);
        return -EINVAL;
    }

    const dmell_cmd_t* command = dmell_find_command( cmd_name );
    if( command != NULL )
    {
        return command->handler( argc, argv );
    }
    else if( g_default_command_handler != NULL )
    {
        return g_default_command_handler( argc, argv );
    }
    else
    {
        DMOD_LOG_ERROR("Command not found and no default handler set in dmell_run_command: %s\n", cmd_name);
        return -ENOENT;
    }
}

/**
 * @brief Runs a command from a command string.
 * 
 * @param cmd Command string to run
 * @param len Length of the command string
 * @return int Exit code of the command
 */
int dmell_run_command_string(const char* cmd, size_t len)
{
    if( cmd == NULL || len == 0 )
    {
        DMOD_LOG_ERROR("Invalid arguments to dmell_run_command_string: %p, %zu\n", cmd, len);
        return -EINVAL;
    }

    dmell_argv_t parsed_argv = {0};
    int result = dmell_parse_command( cmd, len, &parsed_argv );
    if( result < 0 )
    {
        DMOD_LOG_ERROR("Failed to parse command string in dmell_run_command_string\n");
        return result;
    }

    if( parsed_argv.argc == 0 )
    {
        DMOD_LOG_ERROR("No command found in command string\n");
        dmell_free_argv( &parsed_argv );
        return -EINVAL;
    }

    const char* command_name = parsed_argv.argv[0];

    dmell_redirect_backup_t redirect_backup;
    result = dmell_redirect_apply_to_current_process( parsed_argv.redirects, parsed_argv.redirect_count, &redirect_backup );
    if( result < 0 )
    {
        DMOD_LOG_ERROR("Failed to apply stream redirection for command: %s\n", command_name);
        dmell_free_argv( &parsed_argv );
        return result;
    }

    result = dmell_run_command( command_name, parsed_argv.argc, parsed_argv.argv );
    dmell_redirect_restore_current_process( &redirect_backup );

    dmell_free_argv( &parsed_argv );
    return result;
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
        ptr = dmell_skip_whitespaces( ptr, end_ptr );
        if( ptr >= end_ptr )
        {
            break;
        }
        const char* arg = ptr;
        const char* next_arg = get_next_arg(arg, end_ptr);
        result = add_arg( out_argv, arg, next_arg, end_ptr );
        if( result < 0 )
        {
            DMOD_LOG_ERROR("Failed to add argument in dmell_parse_command\n");
            dmell_free_argv( out_argv );
            return result;
        }
        ptr = (next_arg != NULL) ? next_arg : end_ptr;
    }

    result = extract_redirects( out_argv );
    if( result < 0 )
    {
        DMOD_LOG_ERROR("Failed to parse redirection operators in dmell_parse_command\n");
        dmell_free_argv( out_argv );
        return result;
    }

    return 0;
}