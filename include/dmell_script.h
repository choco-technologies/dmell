#ifndef DMELL_SCRIPT_H
#define DMELL_SCRIPT_H

#include "dmell_line.h"
#include "dmell_vars.h"

/** 
 * @brief Maximum length of a script line.
 */
#define DMELL_MAX_SCRIPT_LINE_LENGTH    512 

/** 
 * @brief Context structure for command line execution.
 */
typedef struct 
{
    int last_exit_code;      /**< Exit code of the last executed command */
    dmell_var_t* variables;  /**< Pointer to the head of the variable list */
} dmell_script_ctx_t;

extern int dmell_run_script_line( dmell_script_ctx_t* ctx, const char* line, size_t len );
extern int dmell_run_script_file(const char* file_path, int argc, char** argv);

#endif // DMELL_SCRIPT_H