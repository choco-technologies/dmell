#define DMOD_ENABLE_REGISTRATION    ON

#include <stdint.h>
#include "dmell_proc.h"

/**
 * @file dmell_proc_fake.c
 * @brief Simulated dmosi process backend, built instead of dmell_proc.c when
 *        DMELL_PROC_FAKE is set (wired to DMELL_BUILD_TESTS).
 *
 * No dmell test spawns a real background job today - every test command name
 * is a registered builtin, which dmell_run_background() rejects before ever
 * reaching a real process (see dmell_bg.c) - so there is nothing to actually
 * simulate yet. This exists purely so dmell_bg/dmell_handlers never need a
 * real dmosi backend to be *enabled* just to be loaded by dmod_loader; if a
 * test ever needs real background-job behavior, extend this with an actual
 * fake process table instead of these fixed "already terminated" answers.
 */

dmosi_process_t dmell_proc_find_by_id(dmosi_process_id_t pid)
{
    (void)pid;
    return (dmosi_process_t)(intptr_t)1; // arbitrary non-NULL handle
}

dmosi_process_state_t dmell_proc_get_state(dmosi_process_t proc)
{
    (void)proc;
    return DMOSI_PROCESS_STATE_TERMINATED;
}

void dmell_proc_destroy(dmosi_process_t proc)
{
    (void)proc;
}

int dmell_proc_wait(dmosi_process_t proc, int32_t timeout_ms)
{
    (void)proc;
    (void)timeout_ms;
    return 0;
}

int dmell_proc_get_exit_status(dmosi_process_t proc)
{
    (void)proc;
    return 0;
}

int dmod_init(const Dmod_Config_t *Config)
{
    (void)Config;
    return 0;
}

int dmod_deinit(void)
{
    return 0;
}
