#include <dmod.h>
#include <errno.h>
#include <string.h>

/**
 * @brief Get the type character for a directory entry.
 */
static char entry_type_char(Dmod_DirEntryType_t type)
{
    switch( type )
    {
        case Dmod_DirEntryType_Dir:  return 'd';
        case Dmod_DirEntryType_File: return '-';
        case Dmod_DirEntryType_Link: return 'l';
        case Dmod_DirEntryType_Other:return 'p';
        default:                     return '?';
    }
}

/**
 * @brief Get the filename from a given path.
 */
static const char* get_filename(const char* path)
{
    const char* p = strrchr(path, '/');
    return (p != NULL) ? p + 1 : path;
}

/**
 * @brief List a single path (file or directory).
 */
static int list_path(const char* path, int show_hidden, int long_format)
{
    const char* entry;
    const Dmod_DirEntry_t* entry_ex;
    void* dir = Dmod_OpenDir(path);

    if( dir == NULL )
    {
        /* Try as a regular file. */
        void* file = Dmod_FileOpen(path, "r");
        if( file == NULL )
        {
            Dmod_Printf( "ls: cannot access '%s': No such file or directory\n", path );
            return -1;
        }
        Dmod_FileClose(file);

        const char* name = get_filename(path);
        if( !show_hidden && name[0] == '.' )
            return 0;

        if( long_format )
        {
            Dmod_FileStat_t stat = { 0 };
            (void)Dmod_FileStat(path, &stat);
            Dmod_Printf( "- %-10llu %s\n", (unsigned long long)stat.Size, name );
        }
        else
        {
            Dmod_Printf( "%s\n", name );
        }
        return 0;
    }

    int count = 0;
    while( (entry_ex = Dmod_ReadDirEx(dir)) != NULL )
    {
        entry = entry_ex->name;
        if( !show_hidden && entry[0] == '.' )
            continue;

        if( long_format )
        {
            Dmod_FileStat_t stat = { 0 };
            char full[DMOD_MAX_PATH_LENGTH];
            int off = 0;
            const char* p;
            for( p = path; *p; p++ ) full[off++] = *p;
            full[off++] = '/';
            for( p = entry; *p; p++ ) full[off++] = *p;
            full[off] = '\0';
            uint64_t size = 0;
            (void)Dmod_FileStat(full, &stat);
            if( stat.Size != 0 )
                size = stat.Size;

            char tc = entry_type_char(entry_ex->type);
            if( entry_ex->type == Dmod_DirEntryType_Dir )
                size = 0;

            Dmod_Printf( "%c %-10llu %s\n", tc, (unsigned long long)size, entry );
        }
        else
        {
            Dmod_Printf( "%s  ", entry );
        }
        count++;
    }

    if( !long_format && count > 0 )
        Dmod_Printf( "\n" );

    Dmod_CloseDir(dir);
    return 0;
}

/**
 * @brief Entry point for the 'ls' command module.
 *
 * Lists directory contents or file information.
 * Usage: ls [-a] [-l] [path...]
 */
int main(int argc, char** argv)
{
    int show_hidden = 0;
    int long_format = 0;
    const char* paths[32];
    int path_count = 0;

    /* Parse arguments */
    for( int i = 1; i < argc; i++ )
    {
        if( argv[i][0] == '-' )
        {
            for( int j = 1; argv[i][j] != '\0'; j++ )
            {
                switch( argv[i][j] )
                {
                    case 'a':
                        show_hidden = 1;
                        break;
                    case 'l':
                        long_format = 1;
                        break;
                    default:
                        Dmod_Printf( "ls: unknown option: -%c\n", argv[i][j] );
                        return -EINVAL;
                }
            }
        }
        else
        {
            if( path_count < (int)sizeof(paths)/sizeof(paths[0]) )
                paths[path_count++] = argv[i];
        }
    }

    if( path_count == 0 )
    {
        list_path(".", show_hidden, long_format);
    }
    else
    {
        for( int i = 0; i < path_count; i++ )
        {
            if( path_count > 1 )
                Dmod_Printf( "%s:\n", paths[i] );
            list_path(paths[i], show_hidden, long_format);
        }
    }

    return 0;
}
