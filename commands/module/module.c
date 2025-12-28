#include <dmod.h>
#include <errno.h>
#include <string.h>

/**
 * @brief Print usage information for the module command.
 */
static void print_usage(void)
{
    Dmod_Printf("Usage: module <command> [arguments]\n");
    Dmod_Printf("\nCommands:\n");
    Dmod_Printf("  load <name>     Load a module\n");
    Dmod_Printf("  unload <name>   Unload a module\n");
    Dmod_Printf("  enable <name>   Enable a module\n");
    Dmod_Printf("  disable <name>  Disable a module\n");
    Dmod_Printf("  info <name>     Show module information\n");
    Dmod_Printf("  list            List all available modules\n");
}

/**
 * @brief Load a module by name.
 * 
 * @param module_name Name of the module to load
 * @return int Exit code (0 on success, negative on error)
 */
static int cmd_load(const char* module_name)
{
    if( module_name == NULL || strlen(module_name) == 0 )
    {
        DMOD_LOG_ERROR("Module name is required\n");
        return -EINVAL;
    }

    // Check if module is already loaded
    if( Dmod_IsModuleLoaded(module_name) )
    {
        Dmod_Printf("Module '%s' is already loaded\n", module_name);
        return 0;
    }

    Dmod_Context_t* ctx = Dmod_LoadModuleByName(module_name);
    if( ctx == NULL )
    {
        DMOD_LOG_ERROR("Failed to load module '%s'\n", module_name);
        return -1;
    }

    Dmod_Printf("Module '%s' loaded successfully\n", module_name);
    return 0;
}

/**
 * @brief Unload a module by name.
 * 
 * @param module_name Name of the module to unload
 * @return int Exit code (0 on success, negative on error)
 */
static int cmd_unload(const char* module_name)
{
    if( module_name == NULL || strlen(module_name) == 0 )
    {
        DMOD_LOG_ERROR("Module name is required\n");
        return -EINVAL;
    }

    // Check if module is loaded
    if( !Dmod_IsModuleLoaded(module_name) )
    {
        DMOD_LOG_ERROR("Module '%s' is not loaded\n", module_name);
        return -1;
    }

    bool result = Dmod_UnloadModule(module_name, false);
    if( !result )
    {
        DMOD_LOG_ERROR("Failed to unload module '%s'\n", module_name);
        return -1;
    }

    Dmod_Printf("Module '%s' unloaded successfully\n", module_name);
    return 0;
}

/**
 * @brief Enable a module by name.
 * 
 * @param module_name Name of the module to enable
 * @return int Exit code (0 on success, negative on error)
 */
static int cmd_enable(const char* module_name)
{
    if( module_name == NULL || strlen(module_name) == 0 )
    {
        DMOD_LOG_ERROR("Module name is required\n");
        return -EINVAL;
    }

    // Check if module is already enabled
    if( Dmod_IsModuleEnabled(module_name) )
    {
        Dmod_Printf("Module '%s' is already enabled\n", module_name);
        return 0;
    }

    bool result = Dmod_EnableModule(module_name, false, NULL);
    if( !result )
    {
        DMOD_LOG_ERROR("Failed to enable module '%s'\n", module_name);
        return -1;
    }

    Dmod_Printf("Module '%s' enabled successfully\n", module_name);
    return 0;
}

/**
 * @brief Disable a module by name.
 * 
 * @param module_name Name of the module to disable
 * @return int Exit code (0 on success, negative on error)
 */
static int cmd_disable(const char* module_name)
{
    if( module_name == NULL || strlen(module_name) == 0 )
    {
        DMOD_LOG_ERROR("Module name is required\n");
        return -EINVAL;
    }

    // Check if module is enabled
    if( !Dmod_IsModuleEnabled(module_name) )
    {
        DMOD_LOG_ERROR("Module '%s' is not enabled\n", module_name);
        return -1;
    }

    bool result = Dmod_DisableModule(module_name, false);
    if( !result )
    {
        DMOD_LOG_ERROR("Failed to disable module '%s'\n", module_name);
        return -1;
    }

    Dmod_Printf("Module '%s' disabled successfully\n", module_name);
    return 0;
}

/**
 * @brief Show information about a module.
 * 
 * @param module_name Name of the module
 * @return int Exit code (0 on success, negative on error)
 */
static int cmd_info(const char* module_name)
{
    if( module_name == NULL || strlen(module_name) == 0 )
    {
        DMOD_LOG_ERROR("Module name is required\n");
        return -EINVAL;
    }

    // Find module file path
    char file_path[512];
    bool found = Dmod_FindModuleFile(module_name, NULL, file_path, sizeof(file_path));
    
    if( !found )
    {
        DMOD_LOG_ERROR("Module '%s' not found\n", module_name);
        return -1;
    }

    // Read module header
    Dmod_ModuleHeader_t header;
    if( !Dmod_ReadModuleHeader(file_path, &header) )
    {
        DMOD_LOG_ERROR("Failed to read module header for '%s'\n", module_name);
        return -1;
    }

    // Print module information
    Dmod_Printf("Module Information:\n");
    Dmod_Printf("  Name:         %s\n", header.Name);
    Dmod_Printf("  Version:      %s\n", header.Version);
    Dmod_Printf("  Author:       %s\n", header.Author);
    Dmod_Printf("  Type:         %s\n", 
                header.ModuleType == Dmod_ModuleType_Application ? "Application" :
                header.ModuleType == Dmod_ModuleType_Library ? "Library" : "Unknown");
    Dmod_Printf("  Location:     %s\n", file_path);
    
    // Get file size
    void* file = Dmod_FileOpen(file_path, "r");
    if( file != NULL )
    {
        Dmod_FileSeek(file, 0, DMOD_SEEK_END);
        long size = Dmod_FileTell(file);
        Dmod_FileClose(file);
        Dmod_Printf("  Size:         %ld bytes\n", size);
    }

    // Check module state
    Dmod_Printf("  Loaded:       %s\n", Dmod_IsModuleLoaded(module_name) ? "Yes" : "No");
    Dmod_Printf("  Enabled:      %s\n", Dmod_IsModuleEnabled(module_name) ? "Yes" : "No");
    Dmod_Printf("  Used:         %s\n", Dmod_IsModuleUsed(module_name) ? "Yes" : "No");

    // Print required modules information
    Dmod_Printf("  Required Modules:\n");
    Dmod_RequiredModule_t required_modules[32];
    if( Dmod_ReadRequiredModules(file_path, required_modules, 32) )
    {
        bool has_requirements = false;
        for( int i = 0; i < 32; i++ )
        {
            if( required_modules[i].Name[0] != '\0' )
            {
                has_requirements = true;
                Dmod_Printf("    - %s (version %s)\n", 
                           required_modules[i].Name, 
                           required_modules[i].Version);
            }
        }
        if( !has_requirements )
        {
            Dmod_Printf("    (none)\n");
        }
    }
    else
    {
        Dmod_Printf("    (unable to read)\n");
    }

    return 0;
}

/**
 * @brief List all available modules with their state.
 * 
 * @return int Exit code (0 on success, negative on error)
 */
static int cmd_list(void)
{
    Dmod_Printf("Available Modules:\n");
    Dmod_Printf("%-20s %-10s %-10s %-10s\n", "Name", "Loaded", "Enabled", "Used");
    Dmod_Printf("%-20s %-10s %-10s %-10s\n", "----", "------", "-------", "----");

    // List of known module names to check
    // In a real implementation, we would scan the module directory
    const char* known_modules[] = {
        "dmell", "cp", "mv", "ls", "cat", "mkdir", "touch",
        "head", "tail", "grep", "rm", "rmdir", "find", "which", "printf", "module"
    };
    int num_modules = sizeof(known_modules) / sizeof(known_modules[0]);

    for( int i = 0; i < num_modules; i++ )
    {
        const char* module_name = known_modules[i];
        char file_path[512];
        
        // Check if module file exists
        if( Dmod_FindModuleFile(module_name, NULL, file_path, sizeof(file_path)) )
        {
            bool loaded = Dmod_IsModuleLoaded(module_name);
            bool enabled = Dmod_IsModuleEnabled(module_name);
            bool used = Dmod_IsModuleUsed(module_name);
            
            Dmod_Printf("%-20s %-10s %-10s %-10s\n",
                       module_name,
                       loaded ? "Yes" : "No",
                       enabled ? "Yes" : "No",
                       used ? "Yes" : "No");
        }
    }

    return 0;
}

/**
 * @brief Entry point for the 'module' command module.
 * 
 * Manages DMOD modules (load, unload, enable, disable, info, list).
 * 
 * @param argc Number of arguments
 * @param argv Array of argument strings
 * @return int Exit code (0 on success, negative on error)
 */
int main(int argc, char** argv)
{
    if( argc < 2 )
    {
        print_usage();
        return -EINVAL;
    }

    const char* command = argv[1];

    if( strcmp(command, "load") == 0 )
    {
        if( argc < 3 )
        {
            DMOD_LOG_ERROR("Usage: module load <name>\n");
            return -EINVAL;
        }
        return cmd_load(argv[2]);
    }
    else if( strcmp(command, "unload") == 0 )
    {
        if( argc < 3 )
        {
            DMOD_LOG_ERROR("Usage: module unload <name>\n");
            return -EINVAL;
        }
        return cmd_unload(argv[2]);
    }
    else if( strcmp(command, "enable") == 0 )
    {
        if( argc < 3 )
        {
            DMOD_LOG_ERROR("Usage: module enable <name>\n");
            return -EINVAL;
        }
        return cmd_enable(argv[2]);
    }
    else if( strcmp(command, "disable") == 0 )
    {
        if( argc < 3 )
        {
            DMOD_LOG_ERROR("Usage: module disable <name>\n");
            return -EINVAL;
        }
        return cmd_disable(argv[2]);
    }
    else if( strcmp(command, "info") == 0 )
    {
        if( argc < 3 )
        {
            DMOD_LOG_ERROR("Usage: module info <name>\n");
            return -EINVAL;
        }
        return cmd_info(argv[2]);
    }
    else if( strcmp(command, "list") == 0 )
    {
        return cmd_list();
    }
    else
    {
        DMOD_LOG_ERROR("Unknown command: %s\n", command);
        print_usage();
        return -EINVAL;
    }
}
