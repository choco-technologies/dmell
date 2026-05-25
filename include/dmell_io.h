#ifndef DMELL_IO_H
#define DMELL_IO_H

/**
 * @file dmell_io.h
 * @brief Output I/O helpers for dmell, including stdout redirect support.
 */

/**
 * @brief Set the current stdout redirect file handle.
 *
 * When set to a non-NULL value, dmell_printf writes to this file instead of
 * the terminal. Set back to NULL to restore terminal output.
 *
 * @param file File handle opened with Dmod_FileOpen, or NULL to disable redirect
 */
extern void dmell_set_output_redirect( void* file );

/**
 * @brief Get the current stdout redirect file handle.
 *
 * @return void* Current redirect file handle, or NULL if not redirecting
 */
extern void* dmell_get_output_redirect( void );

/**
 * @brief Print formatted output, respecting any active stdout redirect.
 *
 * Writes to the redirect file if one is set, otherwise writes to the terminal
 * via Dmod_Printf.
 *
 * @param format printf-style format string
 * @param ... Format arguments
 * @return int Number of characters written
 */
extern int dmell_printf( const char* format, ... );

#endif // DMELL_IO_H
