#include <dmod.h>
#include <errno.h>
#include <string.h>

/**
 * @brief Helper function to check if a path is a directory.
 * 
 * @param path Path to check
 * @return bool True if path is a directory, false otherwise
 */
static bool is_dir(const char* path)
{
    void* dir = Dmod_OpenDir(path);
    if(dir != NULL)
    {
        Dmod_CloseDir(dir);
        return true;
    }
    return false;
}

/**
 * @brief Helper function to get the filename from a given path.
 * 
 * @param path Full file path
 * @return const char* Filename extracted from the path
 */
static const char* get_filename_from_path(const char* path)
{
    const char* last_slash = strrchr(path, '/');
    if(last_slash != NULL)
    {
        return last_slash + 1;
    }
    return path;
}

/**
 * @brief Entry point for the 'cp' command module.
 * 
 * Copies a file from source to destination.
 * Usage: cp <source> <destination>
 * 
 * @param argc Number of arguments
 * @param argv Array of argument strings
 * @return int Exit code (0 on success, negative on error)
 */
int Dmod_Main( int argc, char** argv )
{
    if( argc < 3 )
    {
        DMOD_LOG_ERROR("Usage: cp <source> <destination>\n");
        return -EINVAL;
    }

    const char* source = argv[1];
    const char* destination = argv[2];
    bool dest_is_dir = is_dir(destination);
    bool alloced_dest_path = false;
    if( source == NULL || destination == NULL )
    {
        DMOD_LOG_ERROR("Invalid source or destination in cp command\n");
        return -EINVAL;
    }

    if(dest_is_dir)
    {
        const char* filename = get_filename_from_path(source);
        size_t dest_path_len = strlen(destination) + 1 + strlen(filename) + 1;
        char* dest_path = Dmod_Malloc(dest_path_len);
        if(dest_path == NULL)
        {
            DMOD_LOG_ERROR("Memory allocation failed in cp command\n");
            return -ENOMEM;
        }
        Dmod_SnPrintf(dest_path, dest_path_len, "%s/%s", destination, filename);
        destination = dest_path;
        alloced_dest_path = true;
    }

    void* src_file = Dmod_FileOpen(source, "rb");
    if( src_file == NULL )
    {
        DMOD_LOG_ERROR("Failed to open source file '%s'\n", source);
        if(alloced_dest_path)
        {
            Dmod_Free((void*)destination);
        }
        return -1;
    }

    void* dest_file = Dmod_FileOpen(destination, "wb");
    if( dest_file == NULL )
    {
        DMOD_LOG_ERROR("Failed to open destination file '%s'\n", destination);
        Dmod_FileClose(src_file);
        if(alloced_dest_path)
        {
            Dmod_Free((void*)destination);
        }
        return -1;
    }

    char buffer[4096];
    size_t bytes_read;
    while( (bytes_read = Dmod_FileRead(buffer, 1, sizeof(buffer), src_file)) > 0 )
    {
        size_t bytes_written = Dmod_FileWrite(buffer, 1, bytes_read, dest_file);
        if( bytes_written < bytes_read )
        {
            DMOD_LOG_ERROR("Failed to write to destination file '%s'\n", destination);
            Dmod_FileClose(src_file);
            Dmod_FileClose(dest_file);
            if(alloced_dest_path)
            {
                Dmod_Free((void*)destination);
            }
            return -1;
        }
    }

    Dmod_FileClose(src_file);
    Dmod_FileClose(dest_file);
    if(alloced_dest_path)
    {
        Dmod_Free((void*)destination);
    }
    return 0;
}
