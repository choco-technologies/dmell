#ifndef DMELL_CMD_H
#define DMELL_CMD_H

/** 
 * @brief Structure defining a command for the dmell module.
 */
typedef struct 
{
    const char* name;                       /**< Name of the command */
    int (*handler)(int argc, char** argv);  /**< Function pointer to the command handler */
} dmell_cmd_t;

typedef struct 
{
    const char* program_name; /**< Name of the program */
    int argc;       /**< Number of arguments */
    char** argv;    /**< Array of argument strings */
} dmell_argv_t;

extern int dmell_run_command(const char* cmd_name, int argc, char** argv);
extern int dmell_parse_command( const char* cmd, size_t len, dmell_argv_t* out_argv );

#endif // DMELL_CMD_H