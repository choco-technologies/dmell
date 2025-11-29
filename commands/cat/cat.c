#include <dmod.h>
#include <errno.h>
#include <string.h>

/**
 * @brief Entry point for the 'cat' command module.
 * 
 * Concatenates and displays file contents.
 * Usage: cat <file1> [file2 ...]
 * 
 * @param argc Number of arguments
 * @param argv Array of argument strings
 * @return int Exit code (0 on success, negative on error)
 */
int Dmod_Main( int argc, char** argv )
{
    if( argc < 2 )
    {
        DMOD_LOG_ERROR("Usage: cat <file1> [file2 ...]\n");
        return -EINVAL;
    }

    int result = 0;
    for( int i = 1; i < argc; i++ )
    {
        const char* file_name = argv[i];
        void* file = Dmod_FileOpen(file_name, "r");
        if( file == NULL )
        {
            DMOD_LOG_ERROR("Failed to open file '%s'\n", file_name);
            result = -1;
            continue;
        }

        char buffer[4096];
        size_t bytes_read;
        while( (bytes_read = Dmod_FileRead(buffer, 1, sizeof(buffer) - 1, file)) > 0 )
        {
            buffer[bytes_read] = '\0';
            Dmod_Printf("%s", buffer);
        }

        Dmod_FileClose(file);
    }

    return result;
}
