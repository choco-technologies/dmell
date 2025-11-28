#ifndef DMELL_SCRIPT_H
#define DMELL_SCRIPT_H

#include "dmell_line.h"

/** 
 * @brief Context structure for command line execution.
 */
typedef struct 
{
    int last_exit_code;      /**< Exit code of the last executed command */
    int argc;                /**< Number of arguments of the current script */
    char** argv;             /**< Array with arguments of the current script */
} dmell_script_ctx_t;

extern int dmell_run_script_line( dmell_script_ctx_t* ctx, const char* line, size_t len );
extern int dmell_run_script_file(const char* file_path, int argc, char** argv);

#endif // DMELL_SCRIPT_H