#define DMOD_ENABLE_REGISTRATION    ON

#include <string.h>
#include "dmell_core.h"
#include "dmell_cmd.h"
#include "dmell_ia.h"
#include "dmell_script.h"
#include "dmell_vars.h"
#include "dmell_handlers.h"

/**
 * @brief Helper function to print help information.
 */
static void print_help(void)
{
    Dmod_Printf("dmell - A simple command line interpreter module\n");
    Dmod_Printf("Usage: dmell [options]\n");
    Dmod_Printf("Options:\n");
    Dmod_Printf("  -h, --help      Show this help message\n");
    Dmod_Printf("  -v, --version   Show version information\n");
    Dmod_Printf("  -c <cmd>        Execute command string\n");
}

int dmell_main(int argc, char** argv)
{
    int result = -1;
    dmell_ctx_t ctx = { .last_exit_code = 0, .variables = NULL, .bg_jobs = NULL, .bg_job_count = 0, .ia = NULL };
    Dmod_EnvCtx_Push();
    if( argc <= 1 )
    {
        result = dmell_interactive_mode(&ctx);
    }
    else if(argc == 2 && ( strcmp( argv[1], "-h" ) == 0 || strcmp( argv[1], "--help" ) == 0 ) )
    {
        print_help();
        result = 0;
    }
    else if(argc == 2 && ( strcmp( argv[1], "-v" ) == 0 || strcmp( argv[1], "--version" ) == 0 ) )
    {
        Dmod_Printf("dmell version %s\n", DMOD_MODULE_VERSION);
        result = 0;
    }
    else if(argc == 2)
    {
        const char* script_file = argv[1];
        ctx.variables = dmell_add_argv_variables( ctx.variables, argc - 1, &argv[1] );
        result = dmell_run_script_file( &ctx, script_file, argc - 1, &argv[1] );
    }
    else if(argc == 3 && strcmp( argv[1], "-c" ) == 0 )
    {
        result = dmell_run_script_line(&ctx, argv[2], strlen( argv[2] ) );
    }
    else
    {
        Dmod_Printf("Invalid arguments. Use -h or --help for usage information.\n");

        // print arguments
        for( int i = 0; i < argc; i++ )
        {
            Dmod_Printf("argv[%d]: %s\n", i, argv[i]);
        }

        result = -1;
    }
    Dmod_EnvCtx_Pop();
    return result;
}

int dmod_init(const Dmod_Config_t *Config)
{
    (void)Config;
    // Built-in commands are registered once, here, when dmell_core (a
    // singleton Library module) is first loaded - not per dmell process -
    // so the registry never accumulates duplicate entries across instances.
    return dmell_register_handlers();
}

int dmod_deinit(void)
{
    return 0;
}
