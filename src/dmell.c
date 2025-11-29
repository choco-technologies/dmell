#include "dmell.h"
#include <string.h>

/**
 * @brief Helper function to print help information.
 */
static void print_help()
{
    Dmod_Printf("dmell - A simple command line interpreter module\n");
    Dmod_Printf("Usage: dmell [options]\n");
    Dmod_Printf("Options:\n");
    Dmod_Printf("  -h, --help      Show this help message\n");
    Dmod_Printf("  -v, --version   Show version information\n");
    Dmod_Printf("  -c <cmd>        Execute command string\n");
}

/**
 * @brief Main entry point of the module.
 * 
 * @param argc Number of arguments
 * @param argv Array of arguments
 * @return int Exit code
 */
int main(int argc, char** argv)
{
    if( argc <= 1 )
    {
        return dmell_interactive_mode();
    }
    else if(argc == 2 && ( strcmp( argv[1], "-h" ) == 0 || strcmp( argv[1], "--help" ) == 0 ) )
    {
        print_help();
        return 0;
    }
    else if(argc == 2 && ( strcmp( argv[1], "-v" ) == 0 || strcmp( argv[1], "--version" ) == 0 ) )
    {
        Dmod_Printf("dmell version %s\n", DMOD_MODULE_VERSION);
        return 0;
    }
    else if(argc == 3 && strcmp( argv[1], "-c" ) == 0 )
    {
        return dmell_run_line( argv[2], strlen( argv[2] ) );
    }
    else
    {
        Dmod_Printf("Invalid arguments. Use -h or --help for usage information.\n");

        // print arguments
        for( int i = 0; i < argc; i++ )
        {
            Dmod_Printf("argv[%d]: %s\n", i, argv[i]);
        }

        return -1;
    }
}
