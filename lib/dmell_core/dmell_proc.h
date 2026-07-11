#ifndef DMELL_PROC_H
#define DMELL_PROC_H

#include <dmosi.h>

/**
 * @file dmell_proc.h
 * @brief Thin wrapper around dmosi's process-management API.
 *
 * This is the ONLY place in dmell that calls into dmosi's process functions -
 * dmell_bg and dmell_handlers go through here instead of including <dmosi.h>
 * and calling dmosi_process_* directly. That isolates the real "requires a
 * live dmosi backend" dependency to this one small translation unit, so a
 * build can swap in a simulated implementation instead (see
 * dmell_proc_fake.c, selected by the DMELL_PROC_FAKE CMake variable - wired
 * to DMELL_BUILD_TESTS). dmod_test.h test modules run through the generic
 * dmod_loader tool, which has no real dmosi implementation linked in, so
 * without this indirection no test that reaches dmell_bg/dmell_handlers
 * could ever load at all.
 */

extern dmosi_process_t       dmell_proc_find_by_id( dmosi_process_id_t pid );
extern dmosi_process_state_t dmell_proc_get_state( dmosi_process_t proc );
extern void                  dmell_proc_destroy( dmosi_process_t proc );
extern int                   dmell_proc_wait( dmosi_process_t proc, int32_t timeout_ms );
extern int                   dmell_proc_get_exit_status( dmosi_process_t proc );

#endif // DMELL_PROC_H
