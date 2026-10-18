#include <dmod.h>
#include <errno.h>
#include <string.h>

int main(int argc, char** argv)
{
    if( argc == 2 && (strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "-h") == 0) )
    {
        Dmod_Printf("Usage: reboot\nImmediately restart the system. Save pending work first.\n");
        return 0;
    }
    if( argc != 1 )
    {
        DMOD_LOG_ERROR("Usage: reboot\n");
        return -EINVAL;
    }

    int result = Dmod_Reboot();
    // Success does not return. Treat an unexpected nonnegative return as failure.
    DMOD_LOG_ERROR("reboot: system restart failed (%d)\n", result);
    return result < 0 ? result : -EIO;
}
