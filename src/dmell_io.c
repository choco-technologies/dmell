#include <stdarg.h>
#include <dmod.h>
#include "dmell_io.h"

#define DMELL_IO_BUFFER_SIZE 512

/**
 * @brief Current stdout redirect file handle. NULL means write to terminal.
 */
static void* g_dmell_redirect_file = NULL;

/**
 * @brief Set the current stdout redirect file handle.
 *
 * @param file File handle opened with Dmod_FileOpen, or NULL to disable redirect
 */
void dmell_set_output_redirect( void* file )
{
    g_dmell_redirect_file = file;
}

/**
 * @brief Get the current stdout redirect file handle.
 *
 * @return void* Current redirect file handle, or NULL if not redirecting
 */
void* dmell_get_output_redirect( void )
{
    return g_dmell_redirect_file;
}

/**
 * @brief Print formatted output, respecting any active stdout redirect.
 *
 * Uses a stack buffer to format the output and writes it either to the active
 * redirect file (via Dmod_FileWrite) or to the terminal (via Dmod_Printf).
 *
 * @param format printf-style format string
 * @param ... Format arguments
 * @return int Number of characters written
 */
int dmell_printf( const char* format, ... )
{
    char buffer[DMELL_IO_BUFFER_SIZE];
    va_list args;
    va_start( args, format );
    int ret = Dmod_VSnPrintf( buffer, sizeof(buffer), format, args );
    va_end( args );

    if( ret < 0 )
    {
        return ret;
    }

    if( ret == 0 )
    {
        return 0;
    }

    /* Clamp to actual buffer content in case of truncation */
    size_t write_len = ( (size_t)ret < sizeof(buffer) ) ? (size_t)ret : sizeof(buffer) - 1;

    if( g_dmell_redirect_file != NULL )
    {
        size_t written = Dmod_FileWrite( buffer, 1, write_len, g_dmell_redirect_file );
        if( written < write_len )
        {
            return -1;
        }
    }
    else
    {
        Dmod_Printf( "%s", buffer );
    }

    return ret;
}
