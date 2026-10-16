#include <dmod.h>
#include <dmlist.h>
#include <errno.h>
#include <limits.h>
#include <string.h>

typedef struct
{
    bool show_hidden;
    bool directories_only;
    int max_depth;
} tree_options_t;

typedef struct
{
    unsigned int directories;
    unsigned int files;
} tree_counts_t;

typedef struct
{
    char* name;
    bool directory;
} tree_entry_t;

static char* join_path(const char* base, const char* name)
{
    size_t length = strlen(base);
    size_t size = length + strlen(name) + 2;
    char* path = Dmod_Malloc(size);
    if(path != NULL)
    {
        Dmod_SnPrintf(path, size, "%s%s%s", base,
                       length != 0 && base[length - 1] == '/' ? "" : "/", name);
    }
    return path;
}

static char* child_prefix(const char* prefix, bool last)
{
    const char* branch = last ? "    " : "│   ";
    size_t size = strlen(prefix) + strlen(branch) + 1;
    char* result = Dmod_Malloc(size);
    if(result != NULL)
    {
        Dmod_SnPrintf(result, size, "%s%s", prefix, branch);
    }
    return result;
}

static bool is_directory(const char* path, Dmod_DirEntryType_t type)
{
    if(type == Dmod_DirEntryType_Dir) return true;
    if(type != Dmod_DirEntryType_Unknown) return false;
    void* dir = Dmod_OpenDir(path);
    if(dir == NULL) return false;
    Dmod_CloseDir(dir);
    return true;
}

static void free_entries(dmlist_context_t* entries)
{
    tree_entry_t* entry;
    while((entry = dmlist_pop_front(entries)) != NULL)
    {
        Dmod_Free(entry->name);
        Dmod_Free(entry);
    }
    dmlist_destroy(entries);
}

static int compare_entries(const void* left, const void* right)
{
    const tree_entry_t* a = left;
    const tree_entry_t* b = right;
    return strcmp(a->name, b->name);
}

static int collect_entries(void* dir, const char* path, const tree_options_t* options,
                           dmlist_context_t** result)
{
    dmlist_context_t* entries = dmlist_create();
    if(entries == NULL) return -ENOMEM;
    const Dmod_DirEntry_t* item;
    while((item = Dmod_ReadDirEx(dir)) != NULL)
    {
        if(strcmp(item->name, ".") == 0 || strcmp(item->name, "..") == 0) continue;
        if(!options->show_hidden && item->name[0] == '.') continue;
        char* full_path = join_path(path, item->name);
        if(full_path == NULL) { free_entries(entries); return -ENOMEM; }
        bool directory = is_directory(full_path, item->type);
        Dmod_Free(full_path);
        if(options->directories_only && !directory) continue;
        tree_entry_t* entry = Dmod_Malloc(sizeof(*entry));
        if(entry == NULL) { free_entries(entries); return -ENOMEM; }
        entry->name = Dmod_StrDup(item->name);
        entry->directory = directory;
        if(entry->name == NULL || !dmlist_push_back(entries, entry))
        {
            Dmod_Free(entry->name);
            Dmod_Free(entry);
            free_entries(entries);
            return -ENOMEM;
        }
    }
    dmlist_sort(entries, compare_entries);
    *result = entries;
    return 0;
}

static int print_directory(const char* path, const char* prefix, int depth,
                           const tree_options_t* options, tree_counts_t* counts)
{
    void* dir = Dmod_OpenDir(path);
    if(dir == NULL) return -EACCES;
    dmlist_context_t* entries = NULL;
    int result = collect_entries(dir, path, options, &entries);
    Dmod_CloseDir(dir);
    if(result != 0) return result;
    size_t count = dmlist_size(entries);
    for(size_t i = 0; i < count && result == 0; i++)
    {
        tree_entry_t* entry = dmlist_get(entries, i);
        bool last = i + 1 == count;
        Dmod_Printf("%s%s%s\n", prefix, last ? "└── " : "├── ", entry->name);
        if(entry->directory) counts->directories++;
        else counts->files++;
        if(entry->directory && (options->max_depth == 0 || depth < options->max_depth))
        {
            char* child = join_path(path, entry->name);
            char* next_prefix = child_prefix(prefix, last);
            if(child == NULL || next_prefix == NULL) result = -ENOMEM;
            else result = print_directory(child, next_prefix, depth + 1, options, counts);
            Dmod_Free(next_prefix);
            Dmod_Free(child);
        }
    }
    free_entries(entries);
    return result;
}

static int print_path(const char* path, const tree_options_t* options, tree_counts_t* counts)
{
    void* dir = Dmod_OpenDir(path);
    if(dir == NULL)
    {
        void* file = Dmod_FileOpen(path, "r");
        if(file == NULL)
        {
            DMOD_LOG_ERROR("tree: cannot access '%s'\n", path);
            return -ENOENT;
        }
        Dmod_FileClose(file);
        if(!options->directories_only) { Dmod_Printf("%s\n", path); counts->files++; }
        return 0;
    }
    Dmod_CloseDir(dir);
    Dmod_Printf("%s\n", path);
    int result = print_directory(path, "", 1, options, counts);
    if(result != 0) DMOD_LOG_ERROR("tree: cannot read '%s': %d\n", path, result);
    return result;
}

static int parse_level(const char* value, int* level)
{
    int parsed = 0;
    if(*value == '\0') return -EINVAL;
    for(const char* digit = value; *digit != '\0'; digit++)
    {
        if(*digit < '0' || *digit > '9') return -EINVAL;
        int next = *digit - '0';
        if(parsed > (INT_MAX - next) / 10) return -EINVAL;
        parsed = parsed * 10 + next;
    }
    if(parsed == 0) return -EINVAL;
    *level = parsed;
    return 0;
}

static void print_usage(void)
{
    Dmod_Printf("Usage: tree [-a] [-d] [-L level] [path ...]\n");
}

static int parse_options(int argc, char** argv, tree_options_t* options)
{
    bool operands = false;
    for(int i = 1; i < argc; i++)
    {
        if(!operands && strcmp(argv[i], "--") == 0) { operands = true; continue; }
        if(!operands && strcmp(argv[i], "--help") == 0) return 1;
        if(!operands && strcmp(argv[i], "-L") == 0)
        {
            if(++i >= argc || parse_level(argv[i], &options->max_depth) != 0) return -EINVAL;
            continue;
        }
        if(!operands && argv[i][0] == '-' && argv[i][1] != '\0')
        {
            for(const char* flag = argv[i] + 1; *flag != '\0'; flag++)
            {
                if(*flag == 'a') options->show_hidden = true;
                else if(*flag == 'd') options->directories_only = true;
                else return -EINVAL;
            }
        }
    }
    return 0;
}

int main(int argc, char** argv)
{
    tree_options_t options = {0};
    int result = parse_options(argc, argv, &options);
    if(result == 1) { print_usage(); return 0; }
    if(result != 0) { print_usage(); return result; }
    tree_counts_t counts = {0};
    int paths = 0;
    int status = 0;
    bool operands = false;
    for(int i = 1; i < argc; i++)
    {
        if(!operands && strcmp(argv[i], "--") == 0) { operands = true; continue; }
        if(!operands && strcmp(argv[i], "-L") == 0) { i++; continue; }
        if(!operands && argv[i][0] == '-' && argv[i][1] != '\0') continue;
        if(paths++ > 0) Dmod_Printf("\n");
        if(print_path(argv[i], &options, &counts) != 0) status = -1;
    }
    if(paths == 0) status = print_path(".", &options, &counts);
    Dmod_Printf("\n%u directories, %u files\n", counts.directories, counts.files);
    return status;
}
