#ifndef DMELL_IA_H
#define DMELL_IA_H

#include "dmod.h"
#include "dmell_ia_defs.h"
#include "dmell_cmd.h"

/**
 * @brief Enters interactive mode for command input.
 *
 * History (ctx->ia) is allocated lazily on first call - nothing else in the
 * dmell_ctx_t needs it, so a one-shot script or "-c" run never pays for it.
 *
 * @param ctx Per-session context, forwarded to every executed line
 * @return int Exit code
 */
dmod_dmell_ia_global_api( 1.0, int, dmell_interactive_mode, (dmell_ctx_t* ctx) );

#endif // DMELL_IA_H
