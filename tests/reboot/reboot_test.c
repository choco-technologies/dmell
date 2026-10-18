#include "dmod_test.h"
#include <errno.h>

static int reboot_calls;
static int reboot_result;

static int fake_reboot(void)
{
    ++reboot_calls;
    return reboot_result;
}

// Exercise the actual command with a returning platform mock. Never reset the host.
#define Dmod_Reboot fake_reboot
#define main reboot_command_main
#include "../../commands/reboot/reboot.c"
#undef main
#undef Dmod_Reboot

void dmod_test_setup(void)
{
    reboot_calls = 0;
    reboot_result = -ENOSYS;
}

DMOD_TEST_STEP(help_does_not_restart)
{
    char* short_help[] = {"reboot", "-h"};
    char* long_help[] = {"reboot", "--help"};
    DMOD_TEST_EXPECT_EQ(reboot_command_main(2, short_help), 0);
    DMOD_TEST_EXPECT_EQ(reboot_command_main(2, long_help), 0);
    DMOD_TEST_EXPECT_EQ(reboot_calls, 0);
}

DMOD_TEST_STEP(arguments_do_not_restart)
{
    char* invalid[] = {"reboot", "now"};
    char* extra[] = {"reboot", "--help", "extra"};
    DMOD_TEST_EXPECT_EQ(reboot_command_main(2, invalid), -EINVAL);
    DMOD_TEST_EXPECT_EQ(reboot_command_main(3, extra), -EINVAL);
    DMOD_TEST_EXPECT_EQ(reboot_calls, 0);
}

DMOD_TEST_STEP(platform_error_is_preserved)
{
    char* args[] = {"reboot"};
    DMOD_TEST_EXPECT_EQ(reboot_command_main(1, args), -ENOSYS);
    DMOD_TEST_EXPECT_EQ(reboot_calls, 1);
    reboot_result = -EPERM;
    DMOD_TEST_EXPECT_EQ(reboot_command_main(1, args), -EPERM);
    DMOD_TEST_EXPECT_EQ(reboot_calls, 2);
}

DMOD_TEST_STEP(returning_success_is_an_error)
{
    char* args[] = {"reboot"};
    reboot_result = 0;
    DMOD_TEST_EXPECT_EQ(reboot_command_main(1, args), -EIO);
    reboot_result = 1;
    DMOD_TEST_EXPECT_EQ(reboot_command_main(1, args), -EIO);
    DMOD_TEST_EXPECT_EQ(reboot_calls, 2);
}
