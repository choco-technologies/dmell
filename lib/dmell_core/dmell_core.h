#ifndef DMELL_CORE_H
#define DMELL_CORE_H

#include "dmod.h"
#include "dmell_core_defs.h"

/**
 * @brief Runs dmell: parses argv (help/version/script-file/-c/interactive)
 *        and dispatches to the right internal subsystem.
 *
 * This is the one function dmell's own executable calls - everything else in
 * this module (command parsing/dispatch, variables, redirects, background
 * jobs, interactive mode, script execution) is dmell_core's own internal
 * concern, not exposed across a module boundary. All of it used to be split
 * into separate dmod library modules (dmell_vars, dmell_cmd, dmell_line, ...)
 * so it could eventually be loaded lazily piece by piece - merged back into
 * one module for now since dmod has no lazy-loading support yet and the
 * per-module fixed cost (header/footer/signature strings, ~300-500B each)
 * wasn't worth paying ten times over for that not-yet-existing benefit. See
 * the split plan for the exact numbers.
 *
 * @param argc Number of arguments (as passed to main())
 * @param argv Array of argument strings (as passed to main())
 * @return int Process exit code
 */
dmod_dmell_core_global_api( 1.0, int, dmell_main, (int argc, char** argv) );

#endif // DMELL_CORE_H
