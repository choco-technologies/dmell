#include <errno.h>
#include <string.h>
#include <dmod.h>
#include "dmell_redirect.h"

/**
 * @brief Map a dmell stream index to the DMOD standard stream handle.
 */
static void* dmell_stream_to_dmod_handle( dmell_stream_t stream )
{
    switch( stream )
    {
        case DMELL_STREAM_STDIN:  return DMOD_STDIN;
        case DMELL_STREAM_STDOUT: return DMOD_STDOUT;
        case DMELL_STREAM_STDERR: return DMOD_STDERR;
        case DMELL_STREAM_STDLOG: return DMOD_STDLOG;
        default:                  return NULL;
    }
}

bool dmell_redirect_match_token( const char* token, dmell_redirect_match_t* out )
{
    if( token == NULL || out == NULL )
    {
        return false;
    }

    memset( out, 0, sizeof(*out) );

    size_t len = strlen( token );
    size_t p = 0;
    int fd = -1;
    bool combined_prefix = false;

    if( len >= 1 && token[0] >= '0' && token[0] <= '9' )
    {
        fd = token[0] - '0';
        if( fd >= DMELL_STREAM_COUNT )
        {
            return false;
        }
        p = 1;
    }
    else if( len >= 2 && token[0] == '&' && token[1] == '>' )
    {
        combined_prefix = true;
        p = 1;
    }

    if( p >= len )
    {
        return false;
    }

    if( token[p] == '<' )
    {
        if( combined_prefix )
        {
            return false;
        }
        p++;
        out->matched         = true;
        out->op_count        = 1;
        out->stream[0]       = (fd == -1) ? DMELL_STREAM_STDIN : (dmell_stream_t)fd;
        out->attached_target = token + p;
        out->needs_target    = (token[p] == '\0');
        return true;
    }

    if( token[p] != '>' )
    {
        return false;
    }
    p++;

    bool append = false;
    if( p < len && token[p] == '>' )
    {
        append = true;
        p++;
    }

    /* fd-duplication form, e.g. "2>&1": requires an explicit source fd and
     * must consume the token to its very end. */
    if( !combined_prefix && fd != -1 && p + 1 < len && token[p] == '&' &&
        token[p + 1] >= '0' && token[p + 1] <= '9' && p + 2 == len )
    {
        int dup_fd = token[p + 1] - '0';
        if( dup_fd < DMELL_STREAM_COUNT )
        {
            out->matched    = true;
            out->op_count   = 1;
            out->is_dup     = true;
            out->stream[0]  = (dmell_stream_t)fd;
            out->dup_source = (dmell_stream_t)dup_fd;
            return true;
        }
    }

    out->matched         = true;
    out->append          = append;
    out->attached_target = token + p;
    out->needs_target     = (token[p] == '\0');

    if( combined_prefix )
    {
        out->op_count  = 2;
        out->stream[0] = DMELL_STREAM_STDOUT;
        out->stream[1] = DMELL_STREAM_STDERR;
    }
    else
    {
        out->op_count  = 1;
        out->stream[0] = (fd == -1) ? DMELL_STREAM_STDOUT : (dmell_stream_t)fd;
    }

    return true;
}

void dmell_redirect_free( dmell_redirect_t* redirects, size_t count )
{
    if( redirects == NULL )
    {
        return;
    }

    for( size_t i = 0; i < count; i++ )
    {
        Dmod_Free( redirects[i].path );
    }
    Dmod_Free( redirects );
}

int dmell_redirect_resolve( const dmell_redirect_t* redirects, size_t count,
                             Dmod_StreamRedirection_t* out_entries, size_t* out_count )
{
    if( out_entries == NULL || out_count == NULL )
    {
        return -EINVAL;
    }

    const char* target[DMELL_STREAM_COUNT] = { NULL, NULL, NULL, NULL };

    for( size_t i = 0; i < count; i++ )
    {
        const dmell_redirect_t* r = &redirects[i];

        if( r->is_dup )
        {
            target[r->stream] = target[r->dup_source];
            continue;
        }

        target[r->stream] = r->path;

        /* Truncate output targets immediately, like a real shell does before
         * it even knows whether the command can run. DMOD's process streams
         * are always opened in append mode, so truncation is dmell's job. */
        if( r->stream != DMELL_STREAM_STDIN && !r->append )
        {
            void* file = Dmod_FileOpen( r->path, "w" );
            if( file == NULL )
            {
                DMOD_LOG_ERROR("Failed to prepare redirect target: %s\n", r->path);
                return -ENOENT;
            }
            Dmod_FileClose( file );
        }
    }

    size_t n = 0;
    for( int s = 0; s < DMELL_STREAM_COUNT; s++ )
    {
        if( target[s] != NULL )
        {
            out_entries[n].StdHandle = dmell_stream_to_dmod_handle( (dmell_stream_t)s );
            out_entries[n].Path      = target[s];
            n++;
        }
    }

    *out_count = n;
    return 0;
}

void dmell_redirect_restore_current_process( const dmell_redirect_backup_t* backup )
{
    if( backup == NULL || backup->touched_count == 0 )
    {
        return;
    }

    Dmod_Pid_t pid = Dmod_GetCurrentPid();

    for( size_t i = 0; i < backup->touched_count; i++ )
    {
        void* handle = backup->touched[i];
        const char* restore_path = NULL;

        for( size_t j = 0; j < backup->previous_count; j++ )
        {
            if( backup->previous[j].StdHandle == handle )
            {
                restore_path = backup->previous[j].Path;
                break;
            }
        }

        /* restore_path is NULL when the stream was unbound before we touched it:
         * Dmod_SetStreamFilePath treats NULL as "clear the binding", which is
         * exactly what we want here. */
        if( Dmod_SetStreamFilePath( pid, handle, restore_path ) < 0 )
        {
            DMOD_LOG_ERROR("Failed to restore stream binding after redirected command\n");
        }
    }

    for( size_t j = 0; j < backup->previous_count; j++ )
    {
        Dmod_Free( (void*)backup->previous[j].Path );
    }
}

int dmell_redirect_apply_to_current_process( const dmell_redirect_t* redirects, size_t count,
                                              dmell_redirect_backup_t* out_backup )
{
    if( out_backup == NULL )
    {
        return -EINVAL;
    }

    memset( out_backup, 0, sizeof(*out_backup) );

    if( count == 0 )
    {
        return 0;
    }

    if( !Dmod_IsFunctionConnected( (void*)Dmod_SetStreamFilePath ) )
    {
        DMOD_LOG_ERROR("Stream redirection is not supported on this platform\n");
        return -ENOSYS;
    }

    Dmod_StreamRedirection_t new_entries[DMELL_STREAM_COUNT];
    size_t new_count = 0;
    int result = dmell_redirect_resolve( redirects, count, new_entries, &new_count );
    if( result < 0 )
    {
        return result;
    }

    Dmod_Pid_t pid = Dmod_GetCurrentPid();

    result = Dmod_GetStreamRedirections( pid, out_backup->previous, DMELL_STREAM_COUNT, &out_backup->previous_count );
    if( result < 0 )
    {
        DMOD_LOG_ERROR("Failed to snapshot current process streams before redirecting\n");
        out_backup->previous_count = 0;
        return result;
    }

    for( size_t i = 0; i < new_count; i++ )
    {
        out_backup->touched[out_backup->touched_count++] = new_entries[i].StdHandle;

        result = Dmod_SetStreamFilePath( pid, new_entries[i].StdHandle, new_entries[i].Path );
        if( result < 0 )
        {
            DMOD_LOG_ERROR("Failed to redirect stream\n");
            dmell_redirect_restore_current_process( out_backup );
            memset( out_backup, 0, sizeof(*out_backup) );
            return result;
        }
    }

    return 0;
}
