#ifndef DMELL_CMD_H
#define DMELL_CMD_H

#include <stddef.h>

/** 
 * @file dmell_cmd.h
 * @brief Header file for command handling in the dmell module.
 */
typedef int (*dmell_cmd_handler_t)(int argc, char** argv);

/** 
 * @brief Structure defining a command for the dmell module.
 */
typedef struct 
{
    const char*         name;           /**< Name of the command */
    dmell_cmd_handler_t handler;        /**< Function pointer to the command handler */
} dmell_cmd_t;

typedef struct 
{
    const char* program_name; /**< Name of the program */
    int argc;       /**< Number of arguments */
    char** argv;    /**< Array of argument strings */
} dmell_argv_t;

extern int                  dmell_set_default_handler   (dmell_cmd_handler_t handler);
extern int                  dmell_register_command      (const dmell_cmd_t* command);
extern const dmell_cmd_t*   dmell_find_command          (const char* command_name);
extern int                  dmell_unregister_command    (const dmell_cmd_t* command);
extern int                  dmell_run_command           (const char* cmd_name, int argc, char** argv);
extern int                  dmell_run_command_string    (const char* cmd, size_t len);
extern int                  dmell_parse_command         ( const char* cmd, size_t len, dmell_argv_t* out_argv );

#endif // DMELL_CMD_H