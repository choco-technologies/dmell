#include <dmod.h>
#include <dmosi.h>
#include <errno.h>
#include <stdbool.h>
#include <string.h>

/**
 * @brief Convert a thread state enum value to a human-readable string.
 *
 * @param state Thread state
 * @return const char* String representation of the state
 */
static const char* thread_state_str( dmosi_thread_state_t state )
{
    switch( state )
    {
        case DMOSI_THREAD_STATE_CREATED:    return "CREATED";
        case DMOSI_THREAD_STATE_READY:      return "READY";
        case DMOSI_THREAD_STATE_RUNNING:    return "RUNNING";
        case DMOSI_THREAD_STATE_BLOCKED:    return "BLOCKED";
        case DMOSI_THREAD_STATE_SUSPENDED:  return "SUSPENDED";
        case DMOSI_THREAD_STATE_TERMINATED: return "TERMINATED";
        default:                            return "UNKNOWN";
    }
}

/**
 * @brief Convert a process state enum value to a human-readable string.
 *
 * @param state Process state
 * @return const char* String representation of the state
 */
static const char* process_state_str( dmosi_process_state_t state )
{
    switch( state )
    {
        case DMOSI_PROCESS_STATE_CREATED:    return "CREATED";
        case DMOSI_PROCESS_STATE_RUNNING:    return "RUNNING";
        case DMOSI_PROCESS_STATE_SUSPENDED:  return "SUSPENDED";
        case DMOSI_PROCESS_STATE_TERMINATED: return "TERMINATED";
        case DMOSI_PROCESS_STATE_ZOMBIE:     return "ZOMBIE";
        default:                             return "UNKNOWN";
    }
}

/**
 * @brief Entry point for the 'ps' command module.
 *
 * Lists currently running processes and their threads using the dmosi interface.
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
    Dmod_Printf( "%-6s %-20s %-16s %-10s\n",
                 "PID", "PROCESS", "MODULE", "STATE" );
    Dmod_Printf( "  %-22s %-10s %6s\n",
                 "THREAD", "STATE", "CPU%" );

    /* Print each process followed by its threads */
    for( size_t i = 0; i < proc_count; i++ )
    {
        dmosi_process_t        proc      = procs[i];
        const char*            proc_name = dmosi_process_get_name( proc );
        const char*            mod_name  = dmosi_process_get_module_name( proc );
        dmosi_process_state_t  pstate    = dmosi_process_get_state( proc );
        dmosi_process_id_t     pid       = dmosi_process_get_id( proc );

        Dmod_Printf( "%-6u %-20s %-16s %-10s\n",
                     (unsigned)pid,
                     proc_name ? proc_name : "(unknown)",
                     mod_name  ? mod_name  : "(unknown)",
                     process_state_str( pstate ) );

        for( size_t j = 0; j < actual_count; j++ )
        {
            if( dmosi_thread_get_process( threads[j] ) != proc )
            {
                continue;
            }

            const char*       thread_name = dmosi_thread_get_name( threads[j] );
            dmosi_thread_info_t info;
            int ret = dmosi_thread_get_info( threads[j], &info );

            if( ret == 0 )
            {
                Dmod_Printf( "  %-22s %-10s %5.1f%%\n",
                             thread_name ? thread_name : "(unknown)",
                             thread_state_str( info.state ),
                             info.cpu_usage );
            }
            else
            {
                Dmod_Printf( "  %-22s %-10s\n",
                             thread_name ? thread_name : "(unknown)",
                             thread_state_str( DMOSI_THREAD_STATE_CREATED ) );
            }
        }
    }

    Dmod_Free( procs );
    Dmod_Free( threads );
    return 0;
}
