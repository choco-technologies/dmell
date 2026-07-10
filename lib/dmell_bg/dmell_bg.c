#define DMOD_ENABLE_REGISTRATION    ON

#include <errno.h>
#include <string.h>
#include <dmod.h>
#include "dmell_bg.h"
#include "dmell_cmd.h"
#include "dmell_redirect.h"
#include "dmell_hlp.h"
#include "dmell_proc.h"

/**
 * @brief One tracked background job.
 *
 * argv (and any redirect paths in it) must stay valid for as long as the spawned
 * process might still be running (see Dmod_Spawn's argv lifetime requirement), so
 * it is only freed once dmell_bg_reap() has confirmed the process has terminated.
 */
typedef struct
{
    dmosi_process_t proc;
    dmell_argv_t    argv;
} dmell_bg_job_t;

/**
 * @brief Helper function to append a spawned process to the background job list.
 *
 * @param ctx Per-session context whose bg_jobs/bg_job_count are updated
 * @param proc Spawned process handle
 * @param argv Parsed argv/redirects for the job; ownership is transferred on success
 * @return int 0 on success, negative value on error
 */
static int track_job( dmell_ctx_t* ctx, dmosi_process_t proc, dmell_argv_t* argv )
{
    dmell_bg_job_t* jobs = (dmell_bg_job_t*)ctx->bg_jobs;
    dmell_bg_job_t* new_jobs = Dmod_Realloc( jobs, sizeof(dmell_bg_job_t) * (ctx->bg_job_count + 1) );
    if( new_jobs == NULL )
    {
        DMOD_LOG_ERROR("Memory allocation failed while tracking background job\n");
        return -ENOMEM;
    }

    new_jobs[ctx->bg_job_count].proc = proc;
    new_jobs[ctx->bg_job_count].argv = *argv;
    ctx->bg_jobs = new_jobs;
    ctx->bg_job_count++;
    return 0;
}

void dmell_bg_reap( dmell_ctx_t* ctx )
{
    dmell_bg_job_t* jobs = (dmell_bg_job_t*)ctx->bg_jobs;

    for( size_t i = 0; i < ctx->bg_job_count; )
    {
        if( dmell_proc_get_state( jobs[i].proc ) != DMOSI_PROCESS_STATE_TERMINATED )
        {
            i++;
            continue;
        }

        dmell_proc_destroy( jobs[i].proc );
        dmell_free_argv( &jobs[i].argv );

        /* Swap-remove: order among background jobs doesn't matter. */
        jobs[i] = jobs[ctx->bg_job_count - 1];
        ctx->bg_job_count--;
    }

    ctx->bg_jobs = jobs;
}

int dmell_run_background( dmell_ctx_t* ctx, const char* cmd, size_t len )
{
    if( cmd == NULL || len == 0 )
    {
        DMOD_LOG_ERROR("Invalid arguments to dmell_run_background: %p, %zu\n", cmd, len);
        return -EINVAL;
    }

    if( !Dmod_IsFunctionConnected( (void*)Dmod_SpawnModule ) )
    {
        DMOD_LOG_ERROR("Background execution ('&') requires process spawning support, which is not available\n");
        return -ENOSYS;
    }

    dmell_argv_t parsed_argv = {0};
    int result = dmell_parse_command( cmd, len, &parsed_argv );
    if( result < 0 )
    {
        DMOD_LOG_ERROR("Failed to parse command string in dmell_run_background\n");
        return result;
    }

    if( parsed_argv.argc == 0 )
    {
        DMOD_LOG_ERROR("No command found for background execution\n");
        dmell_free_argv( &parsed_argv );
        return -EINVAL;
    }

    const char* command_name = parsed_argv.argv[0];

    if( strchr( command_name, '=' ) != NULL || dmell_find_command( command_name ) != NULL || dmell_has_dme_extension( command_name ) )
    {
        DMOD_LOG_ERROR("Background execution ('&') is only supported for external commands, not '%s'\n", command_name);
        dmell_free_argv( &parsed_argv );
        return -ENOTSUP;
    }

    dmell_redirect_backup_t backup;
    result = dmell_redirect_apply_to_current_process( parsed_argv.redirects, parsed_argv.redirect_count, &backup );
    if( result < 0 )
    {
        DMOD_LOG_ERROR("Failed to apply stream redirection for background command: %s\n", command_name);
        dmell_free_argv( &parsed_argv );
        return result;
    }

    Dmod_StreamRedirection_t entries[DMELL_STREAM_COUNT];
    Dmod_StreamRedirections_t streams = { .Entries = entries, .Count = 0 };
    dmell_redirect_snapshot_current_process( entries, &streams.Count );

    /* Restore dmell's own streams *before* spawning, not after: the snapshot above
     * already captured independent, owned copies of the paths to hand to the child
     * explicitly via Streams, so nothing after this point needs the redirect to
     * still be active on dmell itself. Loading and spawning the module happens
     * synchronously on dmell's own thread (the child's own thread hasn't started
     * yet), and it logs plenty on its own (module loading, API connection, ...) -
     * with the redirect still applied, all of that would go to the child's
     * target instead of the console, and for a file-backed target it can even
     * exhaust the heap writing dmell's own diagnostic output into it. */
    dmell_redirect_restore_current_process( &backup );

    int pid = Dmod_SpawnModule( command_name, parsed_argv.argc, parsed_argv.argv, streams.Count > 0 ? &streams : NULL );

    for( size_t i = 0; i < streams.Count; i++ )
    {
        Dmod_Free( (void*)entries[i].Path );
    }

    if( pid < 0 )
    {
        DMOD_LOG_ERROR("Failed to spawn background command: %s\n", command_name);
        dmell_free_argv( &parsed_argv );
        return pid;
    }

    dmosi_process_t proc = dmell_proc_find_by_id( (dmosi_process_id_t)pid );
    if( proc == NULL )
    {
        DMOD_LOG_ERROR("Failed to find spawned background process: %s\n", command_name);
        dmell_free_argv( &parsed_argv );
        return -ESRCH;
    }

    Dmod_Printf("[bg] started '%s' with pid %d\n", command_name, pid);

    result = track_job( ctx, proc, &parsed_argv );
    if( result < 0 )
    {
        /* The job is already running and argv must stay valid for as long as it might
         * still be reading it, so we deliberately leak parsed_argv here rather than
         * freeing memory out from under it - it will just never be reaped. */
        DMOD_LOG_ERROR("Background job '%s' is running untracked and will not be cleaned up\n", command_name);
        return 0;
    }

    return 0;
}

int dmod_init(const Dmod_Config_t *Config)
{
    (void)Config;
    return 0;
}

int dmod_deinit(void)
{
    return 0;
}
