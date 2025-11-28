#ifndef DMELL_LINE_H
#define DMELL_LINE_H

#include "dmell_cmd.h"

/**
 * @brief Enumeration of command line separators.
 */
typedef enum 
{
    dmell_line_sep_none,    //!< No separator
    dmell_line_sep_and,     //!< '&&' separator
    dmell_line_sep_or,      //!< '||' separator
    dmell_line_sep_seq,     //!< Semicolon or newline separator

    dmell_line_sep_max      //!< Maximum value for validation
} dmell_line_sep_t;

extern int dmell_run_line(const char* line, size_t len);
extern int dmell_run_args_line(int argc, char** argv);

#endif // DMELL_LINE_H