#include <dmod.h>
#include <errno.h>
#include <string.h>

/**
 * @brief Entry point for the 'printf' command module.
 * 
 * Formats and prints text.
 * Usage: printf <format> [arguments...]
 * 
 * @param argc Number of arguments
 * @param argv Array of argument strings
 * @return int Exit code (0 on success, negative on error)
 */
int main( int argc, char** argv )
{
    if( argc < 2 )
    {
        DMOD_LOG_ERROR("Usage: printf <format> [arguments...]\n");
        return -EINVAL;
    }

    const char* format = argv[1];
    int arg_index = 2;
    
    // Simple printf implementation with basic format specifiers
    for( const char* p = format; *p != '\0'; p++ )
    {
        if( *p == '\\' )
        {
            // Handle escape sequences
            p++;
            switch( *p )
            {
                case 'n':
                    Dmod_Printf("\n");
                    break;
                case 't':
                    Dmod_Printf("\t");
                    break;
                case 'r':
                    Dmod_Printf("\r");
                    break;
                case '\\':
                    Dmod_Printf("\\");
                    break;
                case '0':
                    // Null character - end of output
                    return 0;
                default:
                    Dmod_Printf("\\%c", *p);
                    break;
            }
        }
        else if( *p == '%' )
        {
            // Handle format specifiers
            p++;
            if( *p == '\0' )
            {
                Dmod_Printf("%%");
                break;
            }
            
            switch( *p )
            {
                case 's':
                    if( arg_index < argc )
                    {
                        Dmod_Printf("%s", argv[arg_index++]);
                    }
                    break;
                case 'd':
                case 'i':
                    if( arg_index < argc )
                    {
                        // String to int conversion with overflow protection
                        const char* arg = argv[arg_index++];
                        int val = 0;
                        int sign = 1;
                        if( *arg == '-' )
                        {
                            sign = -1;
                            arg++;
                        }
                        else if( *arg == '+' )
                        {
                            arg++;
                        }
                        while( *arg >= '0' && *arg <= '9' )
                        {
                            // Overflow check
                            if( val > (2147483647 - (*arg - '0')) / 10 )
                            {
                                val = 2147483647;
                                break;
                            }
                            val = val * 10 + (*arg - '0');
                            arg++;
                        }
                        Dmod_Printf("%d", val * sign);
                    }
                    break;
                case 'x':
                    if( arg_index < argc )
                    {
                        // Parse decimal input and output as hex
                        const char* arg = argv[arg_index++];
                        unsigned int val = 0;
                        while( *arg >= '0' && *arg <= '9' )
                        {
                            // Overflow check
                            if( val > (0xFFFFFFFF - (*arg - '0')) / 10 )
                            {
                                val = 0xFFFFFFFF;
                                break;
                            }
                            val = val * 10 + (*arg - '0');
                            arg++;
                        }
                        Dmod_Printf("%x", val);
                    }
                    break;
                case 'X':
                    if( arg_index < argc )
                    {
                        // Parse decimal input and output as hex uppercase
                        const char* arg = argv[arg_index++];
                        unsigned int val = 0;
                        while( *arg >= '0' && *arg <= '9' )
                        {
                            // Overflow check
                            if( val > (0xFFFFFFFF - (*arg - '0')) / 10 )
                            {
                                val = 0xFFFFFFFF;
                                break;
                            }
                            val = val * 10 + (*arg - '0');
                            arg++;
                        }
                        Dmod_Printf("%X", val);
                    }
                    break;
                case 'c':
                    if( arg_index < argc )
                    {
                        Dmod_Printf("%c", argv[arg_index++][0]);
                    }
                    break;
                case '%':
                    Dmod_Printf("%%");
                    break;
                default:
                    Dmod_Printf("%%%c", *p);
                    break;
            }
        }
        else
        {
            Dmod_Printf("%c", *p);
        }
    }

    return 0;
}
