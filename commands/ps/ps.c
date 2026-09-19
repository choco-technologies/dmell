#include <dmod.h>
#include <dmosi.h>
#include <errno.h>
#include <stdbool.h>
#include <string.h>

/**
 * @brief Convert a thread state enum value to a Linux ps-style STAT letter.
 *
 * @param state Thread state
 * @return char Single-letter state code
 */
static char thread_state_char( dmosi_thread_state_t state )
{
    switch( state )
    {
        case DMOSI_THREAD_STATE_CREATED:    return 'I';
        case DMOSI_THREAD_STATE_READY:      return 'R';
        case DMOSI_THREAD_STATE_RUNNING:    return 'R';
        case DMOSI_THREAD_STATE_BLOCKED:    return 'S';
        case DMOSI_THREAD_STATE_SUSPENDED:  return 'T';
        case DMOSI_THREAD_STATE_TERMINATED: return 'X';
        default:                            return '?';
    }
}

/**
 * @brief Convert a process state enum value to a Linux ps-style STAT letter.
 *
 * @param state Process state
 * @return char Single-letter state code
 */
static char process_state_char( dmosi_process_state_t state )
{
    switch( state )
    {
        case DMOSI_PROCESS_STATE_CREATED:    return 'I';
        case DMOSI_PROCESS_STATE_RUNNING:    return 'R';
        case DMOSI_PROCESS_STATE_SUSPENDED:  return 'T';
        case DMOSI_PROCESS_STATE_TERMINATED: return 'X';
        case DMOSI_PROCESS_STATE_ZOMBIE:     return 'Z';
        default:                             return '?';
    }
}

/**
 * @brief Format a runtime in milliseconds as a Linux ps-style HH:MM:SS field.
 *
 * @param runtime_ms Runtime in milliseconds
 * @param buf        Output buffer
 * @param buf_size   Size of @p buf
 */
static void format_time( uint64_t runtime_ms, char* buf, size_t buf_size )
{
    uint64_t total_seconds = runtime_ms / 1000;
    unsigned hours   = (unsigned)( total_seconds / 3600 );
    unsigned minutes = (unsigned)( ( total_seconds % 3600 ) / 60 );
    unsigned seconds = (unsigned)( total_seconds % 60 );

    Dmod_SnPrintf( buf, buf_size, "%02u:%02u:%02u", hours, minutes, seconds );
}

/**
 * @brief Build the COMMAND field for a process: its name, plus the owning
 *        module name in brackets when it differs from the process name.
 *
 * @param proc     Process handle
 * @param buf      Output buffer
 * @param buf_size Size of @p buf
 */
static void format_command( dmosi_process_t proc, char* buf, size_t buf_size )
{
    const char* name = dmosi_process_get_name( proc );
    const char* mod  = dmosi_process_get_module_name( proc );

    name = name ? name : "?";

    if( mod != NULL && strcmp( mod, name ) != 0 )
    {
        Dmod_SnPrintf( buf, buf_size, "%s [%s]", name, mod );
    }
    else
    {
        Dmod_SnPrintf( buf, buf_size, "%s", name );
    }
}

/**
 * @brief Entry point for the 'ps' command module.
 *
 * Lists currently running processes and their threads using the dmosi
 * interface, formatted like Linux's `ps -eLf`: one summary line per
 * process followed by a tree of its threads.
 * Usage: ps
 *
 * @param argc Number of arguments
 * @param argv Array of argument strings
 * @return int Exit code (0 on success, negative on error)
 */
int main( int argc, char** argv )
{
    (void)argc;
    (void)argv;

    /* Query the total number of threads */
    size_t thread_count = dmosi_thread_get_all( NULL, 0 );
    if( thread_count == 0 )
    {
        Dmod_Printf( "No threads running.\n" );
        return 0;
    }

    /* Allocate storage for thread handles */
    dmosi_thread_t* threads = Dmod_Malloc( thread_count * sizeof( dmosi_thread_t ) );
    if( threads == NULL )
    {
        DMOD_LOG_ERROR( "Failed to allocate memory for thread list\n" );
        return -ENOMEM;
    }

    /* Fetch all thread handles */
    size_t actual_count = dmosi_thread_get_all( threads, thread_count );

    /* Collect unique process handles (at most one per thread) */
    dmosi_process_t* procs = Dmod_Malloc( actual_count * sizeof( dmosi_process_t ) );
    if( procs == NULL )
    {
        Dmod_Free( threads );
        DMOD_LOG_ERROR( "Failed to allocate memory for process list\n" );
        return -ENOMEM;
    }

    size_t proc_count = 0;
    for( size_t i = 0; i < actual_count; i++ )
    {
        dmosi_process_t proc = dmosi_thread_get_process( threads[i] );
        bool found = false;
        for( size_t j = 0; j < proc_count; j++ )
        {
            if( procs[j] == proc )
            {
                found = true;
                break;
            }
        }
        if( !found )
        {
            procs[proc_count++] = proc;
        }
    }

    /* Print table header */
    Dmod_Printf( "%5s %5s %5s %-5s %6s %8s %-20s %s\n",
                 "PID", "PPID", "UID", "STAT", "%CPU", "TIME", "COMMAND", "CMD" );

    /* Print each process followed by a tree of its threads */
    for( size_t i = 0; i < proc_count; i++ )
    {
        dmosi_process_t        proc      = procs[i];
        dmosi_process_t        parent    = dmosi_process_get_parent( proc );
        dmosi_process_id_t     pid       = dmosi_process_get_id( proc );
        dmosi_process_id_t     ppid      = parent ? dmosi_process_get_id( parent ) : 0;
        dmosi_user_id_t        uid       = dmosi_process_get_uid( proc );
        dmosi_process_state_t  pstate    = dmosi_process_get_state( proc );

        char cmd_buf[64];
        format_command( proc, cmd_buf, sizeof( cmd_buf ) );

        /* Full command line (program plus arguments) the process was started with,
         * as recorded by the module-start API via dmosi_process_set_command() -
         * unavailable for processes not spawned through it (e.g. init). */
        const char* full_command = dmosi_process_get_command( proc );
        full_command = full_command ? full_command : "-";

        char pstat_str[2] = { process_state_char( pstate ), '\0' };

        /* Aggregate CPU% and runtime across the process's own threads */
        float    total_cpu = 0.0f;
        uint64_t total_runtime_ms = 0;
        size_t   proc_thread_count = 0;

        for( size_t j = 0; j < actual_count; j++ )
        {
            if( dmosi_thread_get_process( threads[j] ) != proc )
            {
                continue;
            }

            dmosi_thread_info_t info;
            if( dmosi_thread_get_info( threads[j], &info ) == 0 )
            {
                total_cpu += info.cpu_usage;
                total_runtime_ms += info.runtime_ms;
            }
            proc_thread_count++;
        }

        char time_buf[16];
        format_time( total_runtime_ms, time_buf, sizeof( time_buf ) );

        Dmod_Printf( "%5u %5u %5u %-5s %6.1f %8s %-20s %s\n",
                     (unsigned)pid,
                     (unsigned)ppid,
                     (unsigned)uid,
                     pstat_str,
                     (double)total_cpu,
                     time_buf,
                     cmd_buf,
                     full_command );

        /* Print the thread tree for this process */
        size_t printed = 0;
        for( size_t j = 0; j < actual_count; j++ )
        {
            if( dmosi_thread_get_process( threads[j] ) != proc )
            {
                continue;
            }

            printed++;
            const char* connector = ( printed == proc_thread_count ) ? "\xe2\x94\x94\xe2\x94\x80 " /* '└─ ' */
                                                                       : "\xe2\x94\x9c\xe2\x94\x80 " /* '├─ ' */;

            const char* thread_name = dmosi_thread_get_name( threads[j] );
            dmosi_thread_info_t info;
            char thread_time_buf[16];
            char tstat_str[2];
            double cpu = 0.0;

            if( dmosi_thread_get_info( threads[j], &info ) == 0 )
            {
                tstat_str[0] = thread_state_char( info.state );
                cpu = (double)info.cpu_usage;
                format_time( info.runtime_ms, thread_time_buf, sizeof( thread_time_buf ) );
            }
            else
            {
                tstat_str[0] = thread_state_char( DMOSI_THREAD_STATE_CREATED );
                format_time( 0, thread_time_buf, sizeof( thread_time_buf ) );
            }
            tstat_str[1] = '\0';

            Dmod_Printf( "%5s %5s %5s %-5s %6.1f %8s   %s%s\n",
                         "", "", "",
                         tstat_str,
                         cpu,
                         thread_time_buf,
                         connector,
                         thread_name ? thread_name : "(unknown)" );
        }
    }

    Dmod_Free( procs );
    Dmod_Free( threads );
    return 0;
}
