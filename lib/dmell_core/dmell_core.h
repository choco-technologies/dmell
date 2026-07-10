#ifndef DMELL_CORE_H
#define DMELL_CORE_H

#include "dmod.h"
#include "dmell_core_defs.h"

/**
 * @brief Runs dmell: parses argv (help/version/script-file/-c/interactive),
 *        owns the per-session dmell_ctx_t, and dispatches to the right
 *        subsystem (dmell_ia, dmell_script, dmell_handlers, ...).
 *
 * This is the one place dmell's own executable ever calls into - every other
 * dmell_* module is this library's own concern, not the executable's. That is
 * deliberate: once dmod supports on-demand module loading, this is where a
 * given subsystem (e.g. dmell_ia, only needed for the interactive branch)
 * would be loaded lazily instead of being an unconditional required-module
 * dependency - without dmell's own executable ever having to change.
 *
 * @param argc Number of arguments (as passed to main())
 * @param argv Array of argument strings (as passed to main())
 * @return int Process exit code
 */
dmod_dmell_core_global_api( 1.0, int, dmell_main, (int argc, char** argv) );

#endif // DMELL_CORE_H
