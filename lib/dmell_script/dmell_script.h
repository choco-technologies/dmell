#ifndef DMELL_SCRIPT_H
#define DMELL_SCRIPT_H

#include "dmod.h"
#include "dmell_script_defs.h"
#include "dmell_line.h"

/**
 * @brief Maximum length of a script line.
 */
#define DMELL_MAX_SCRIPT_LINE_LENGTH    512

/**
 * @brief Executes a line of commands in the context of a script, with variable expansion.
 *
 * @param ctx Per-session context: ctx->variables (dmell_var_t*) is used for
 *            expansion and updated with the resulting "?" exit-code variable,
 *            ctx->last_exit_code is updated with the line's exit code.
 * @param line Command line string
 * @param len Length of the command line string
 * @return int Exit code of the last executed command, or negative value on error
 */
dmod_dmell_script_global_api( 1.0, int, dmell_run_script_line, (dmell_ctx_t* ctx, const char* line, size_t len) );

/**
 * @brief Executes a script file with given arguments.
 *
 * @param ctx Per-session context, forwarded to dmell_run_script_line() for every line
 * @param file_path Path to the script file
 * @param argc Number of arguments
 * @param argv Array of argument strings
 * @return int Exit code of the script execution, or negative value on error
 */
dmod_dmell_script_global_api( 1.0, int, dmell_run_script_file, (dmell_ctx_t* ctx, const char* file_path, int argc, char** argv) );

#endif // DMELL_SCRIPT_H
