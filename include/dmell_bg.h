#ifndef DMELL_BG_H
#define DMELL_BG_H

#include <stddef.h>

/**
 * @file dmell_bg.h
 * @brief Background command execution support for dmell (a trailing '&', like a POSIX shell).
 *
 * Only external DMOD module commands can be backgrounded: built-ins, variable
 * assignment, and .dme scripts/shebang interpreters all execute inside dmell's own
 * process/thread (see dmell_redirect.h) and have no independent execution context
 * to hand off without blocking the shell, so those are rejected with an error.
 */

/**
 * @brief Run a command in the background and return immediately.
 *
 * Parses @p cmd exactly like dmell_run_command_string() (including any redirection
 * operators), spawns it as a new DMOD process, and returns without waiting for it
 * to finish. The spawned process is tracked internally; call dmell_bg_reap()
 * periodically to release resources for jobs that have completed.
 *
 * @param cmd Command string (trailing '&' already stripped by the caller)
 * @param len Length of the command string
 * @return 0 if the command was launched successfully, negative errno value otherwise
 */
extern int dmell_run_background( const char* cmd, size_t len );

/**
 * @brief Release resources held by background jobs that have finished.
 *
 * Cheap to call even when there are no background jobs, or none have finished yet.
 */
extern void dmell_bg_reap( void );

#endif // DMELL_BG_H
