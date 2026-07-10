#ifndef DMELL_CMD_H
#define DMELL_CMD_H

#include <stddef.h>
#include <stdbool.h>
#include "dmod.h"
#include "dmell_cmd_defs.h"
#include "dmell_redirect.h"

/**
 * @file dmell_cmd.h
 * @brief Header file for command handling in the dmell module.
 */

/**
 * @brief Per-session state threaded through the whole command execution chain
 *        (dmell_cmd -> dmell_line -> dmell_script -> dmell_bg).
 *
 * Kept here (rather than in each owning module's own header) because
 * dmell_cmd_handler_t needs the type and dmell_cmd sits at the bottom of the
 * dependency graph. `variables` and `bg_jobs` are opaque (void*) on purpose:
 * dmell_cmd itself never looks inside them, only threads the pointer through -
 * their real element types (dmell_var_t*, dmell_bg_job_t*) are owned by
 * dmell_vars/dmell_script and dmell_bg respectively, which cast accordingly.
 *
 * One instance lives for the lifetime of a single dmell process/session -
 * never as static/global storage in a library, since a Library module's
 * storage is a single system-wide singleton shared by every dmell instance.
 */
typedef struct dmell_ctx_s
{
    int    last_exit_code;  /**< Exit code of the last executed command; owned by dmell_script */
    void*  variables;       /**< dmell_var_t* list head; owned by dmell_script/dmell_vars */
    void*  bg_jobs;         /**< dmell_bg_job_t* array; owned by dmell_bg */
    size_t bg_job_count;    /**< Number of entries in bg_jobs; owned by dmell_bg */
    void*  ia;              /**< Interactive-mode state (history, ...); owned by dmell_ia, lazily allocated on first use */
} dmell_ctx_t;

typedef int (*dmell_cmd_handler_t)(int argc, char** argv, dmell_ctx_t* ctx);

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
    const char*        program_name;    /**< Name of the program */
    int                argc;            /**< Number of arguments */
    char**             argv;            /**< Array of argument strings */
    dmell_redirect_t*  redirects;       /**< Stream redirections parsed from the command, or NULL if none */
    size_t             redirect_count;  /**< Number of entries in redirects */
} dmell_argv_t;

dmod_dmell_cmd_global_api( 1.0, int,               dmell_set_default_handler,     (dmell_cmd_handler_t handler) );
dmod_dmell_cmd_global_api( 1.0, int,               dmell_register_command,        (const dmell_cmd_t* command) );
dmod_dmell_cmd_global_api( 1.0, int,               dmell_register_command_handler,(const char* command_name, dmell_cmd_handler_t handler) );
dmod_dmell_cmd_global_api( 1.0, const dmell_cmd_t*, dmell_find_command,           (const char* command_name) );
dmod_dmell_cmd_global_api( 1.0, int,               dmell_unregister_command,      (const dmell_cmd_t* command) );
dmod_dmell_cmd_global_api( 1.0, int,               dmell_run_command,             (dmell_ctx_t* ctx, const char* cmd_name, int argc, char** argv) );
dmod_dmell_cmd_global_api( 1.0, int,               dmell_run_command_string,      (dmell_ctx_t* ctx, const char* cmd, size_t len) );
dmod_dmell_cmd_global_api( 1.0, int,               dmell_parse_command,           (const char* cmd, size_t len, dmell_argv_t* out_argv) );
dmod_dmell_cmd_global_api( 1.0, void,              dmell_free_argv,               (dmell_argv_t* argv) );

/**
 * @brief Finds the first registered built-in command whose name starts with @p partial_name.
 *
 * Used by dmell_ia's tab completion, which lives in a different module and so can no
 * longer reach the command registry's storage directly (it used to `extern` the raw
 * arrays before the registry moved into this library).
 *
 * @param partial_name Partial command name to match (non-empty)
 * @param out_match    Output buffer for the matching command name
 * @param max_length   Size of out_match
 * @return true if a match was found and copied into out_match, false otherwise
 */
dmod_dmell_cmd_global_api( 1.0, bool, dmell_find_command_prefix_match, (const char* partial_name, char* out_match, size_t max_length) );

#endif // DMELL_CMD_H
