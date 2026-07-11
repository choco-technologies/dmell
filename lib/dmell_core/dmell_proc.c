#include "dmell_proc.h"

/**
 * @file dmell_proc.c
 * @brief Real implementation: forwards straight to dmosi's process API.
 */

dmosi_process_t dmell_proc_find_by_id(dmosi_process_id_t pid)
{
    return dmosi_process_find_by_id(pid);
}

dmosi_process_state_t dmell_proc_get_state(dmosi_process_t proc)
{
    return dmosi_process_get_state(proc);
}

void dmell_proc_destroy(dmosi_process_t proc)
{
    dmosi_process_destroy(proc);
}

int dmell_proc_wait(dmosi_process_t proc, int32_t timeout_ms)
{
    return dmosi_process_wait(proc, timeout_ms);
}

int dmell_proc_get_exit_status(dmosi_process_t proc)
{
    return dmosi_process_get_exit_status(proc);
}
