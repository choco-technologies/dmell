#include <errno.h>
#include <string.h>
#include <dmod.h>
#include <dmosi.h>
#include "dmell_bg.h"
#include "dmell_cmd.h"
#include "dmell_redirect.h"

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

static dmell_bg_job_t* g_bg_jobs = NULL;
static size_t           g_bg_job_count = 0;

/**
 * @brief Helper function to check if a file is a dmell script based on its extension.
 *
 * Mirrors dmell_handlers.c's is_dmell_script() - duplicated rather than exported,
 * since it is the only piece of that file's module-resolution logic needed here:
 * a .dme script has no Dmod_Context, so Dmod_SpawnModule can never load it, and
 * this exists purely to reject that case with a precise error instead of a
 * confusing generic "failed to load module" one.
 *
 * @param file_name Name of the file
 * @return bool True if it is a dmell script, false otherwise
 */
static bool has_dme_extension( const char* file_name )
{
    size_t len = strlen( file_name );
    return ( len > 4 && strcmp( &file_name[len - 4], ".dme" ) == 0 );
}

/**
 * @brief Helper function to append a spawned process to the background job list.
 *
 * @param proc Spawned process handle
 * @param argv Parsed argv/redirects for the job; ownership is transferred on success
 * @return int 0 on success, negative value on error
 */
static int track_job( dmosi_process_t proc, dmell_argv_t* argv )
{
    dmell_bg_job_t* new_jobs = Dmod_Realloc( g_bg_jobs, sizeof(dmell_bg_job_t) * (g_bg_job_count + 1) );
    if( new_jobs == NULL )
    {
        DMOD_LOG_ERROR("Memory allocation failed while tracking background job\n");
        return -ENOMEM;
    }

    g_bg_jobs = new_jobs;
    g_bg_jobs[g_bg_job_count].proc = proc;
    g_bg_jobs[g_bg_job_count].argv = *argv;
    g_bg_job_count++;
    return 0;
}

void dmell_bg_reap( void )
{
    for( size_t i = 0; i < g_bg_job_count; )
    {
        if( dmosi_process_get_state( g_bg_jobs[i].proc ) != DMOSI_PROCESS_STATE_TERMINATED )
        {
            i++;
            continue;
        }

        dmosi_process_destroy( g_bg_jobs[i].proc );
        dmell_free_argv( &g_bg_jobs[i].argv );

        /* Swap-remove: order among background jobs doesn't matter. */
        g_bg_jobs[i] = g_bg_jobs[g_bg_job_count - 1];
        g_bg_job_count--;
    }
}

int dmell_run_background( const char* cmd, size_t len )
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

    if( strchr( command_name, '=' ) != NULL || dmell_find_command( command_name ) != NULL || has_dme_extension( command_name ) )
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

    int pid = Dmod_SpawnModule( command_name, parsed_argv.argc, parsed_argv.argv, streams.Count > 0 ? &streams : NULL );

    for( size_t i = 0; i < streams.Count; i++ )
    {
        Dmod_Free( (void*)entries[i].Path );
    }

    dmell_redirect_restore_current_process( &backup );

    if( pid < 0 )
    {
        DMOD_LOG_ERROR("Failed to spawn background command: %s\n", command_name);
        dmell_free_argv( &parsed_argv );
        return pid;
    }

    dmosi_process_t proc = dmosi_process_find_by_id( (dmosi_process_id_t)pid );
    if( proc == NULL )
    {
        DMOD_LOG_ERROR("Failed to find spawned background process: %s\n", command_name);
        dmell_free_argv( &parsed_argv );
        return -ESRCH;
    }

    Dmod_Printf("[bg] started '%s' with pid %d\n", command_name, pid);

    result = track_job( proc, &parsed_argv );
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
