#ifndef DMELL_BG_H
#define DMELL_BG_H

#include <stddef.h>
#include "dmell_cmd.h"

/**
 * @file dmell_bg.h
 * @brief Background command execution support for dmell (a trailing '&', like a POSIX shell).
 *
 * Only external DMOD module commands can be backgrounded: built-ins, variable
 * assignment, and .dme scripts/shebang interpreters all execute inside dmell's own
 * process/thread (see dmell_redirect.h) and have no independent execution context
 * to hand off without blocking the shell, so those are rejected with an error.
 *
 * The job list itself lives in the caller's dmell_ctx_t (ctx->bg_jobs/bg_job_count),
 * not in a static/global here: dmell_core is a Library module, a single
 * system-wide singleton shared by every dmell process, so any state that
 * genuinely differs per session (as the set of running background jobs does)
 * has to be threaded through explicitly instead.
 */

/**
 * @brief Run a command in the background and return immediately.
 *
 * Parses @p cmd exactly like dmell_run_command_string() (including any redirection
 * operators), spawns it as a new DMOD process, and returns without waiting for it
 * to finish. The spawned process is tracked in ctx->bg_jobs; call dmell_bg_reap()
 * periodically to release resources for jobs that have completed.
 *
 * @param ctx Per-session context; its bg_jobs/bg_job_count fields are updated
 * @param cmd Command string (trailing '&' already stripped by the caller)
 * @param len Length of the command string
 * @return 0 if the command was launched successfully, negative errno value otherwise
 */
extern int dmell_run_background( dmell_ctx_t* ctx, const char* cmd, size_t len );

/**
 * @brief Release resources held by background jobs that have finished.
 *
 * Cheap to call even when there are no background jobs, or none have finished yet.
 *
 * @param ctx Per-session context whose bg_jobs/bg_job_count are inspected and updated
 */
extern void dmell_bg_reap( dmell_ctx_t* ctx );

#endif // DMELL_BG_H
