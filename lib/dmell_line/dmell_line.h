#ifndef DMELL_LINE_H
#define DMELL_LINE_H

#include "dmod.h"
#include "dmell_line_defs.h"
#include "dmell_cmd.h"

/**
 * @brief Enumeration of command line separators.
 */
typedef enum
{
    dmell_line_sep_none,       //!< No separator
    dmell_line_sep_and,        //!< '&&' separator
    dmell_line_sep_or,         //!< '||' separator
    dmell_line_sep_seq,        //!< Semicolon or newline separator
    dmell_line_sep_background, //!< Single '&' - run the preceding command in the background

    dmell_line_sep_max      //!< Maximum value for validation
} dmell_line_sep_t;

dmod_dmell_line_global_api( 1.0, int, dmell_run_line,      (dmell_ctx_t* ctx, const char* line, size_t len) );
dmod_dmell_line_global_api( 1.0, int, dmell_run_args_line, (dmell_ctx_t* ctx, int argc, char** argv) );

#endif // DMELL_LINE_H
