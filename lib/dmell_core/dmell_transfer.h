#ifndef DMELL_TRANSFER_H
#define DMELL_TRANSFER_H

#include "dmell_cmd.h"

/* Foreground built-ins: use the shell's existing TTY handle/ownership. */
int dmell_handler_sendf(int argc, char **argv, dmell_ctx_t *ctx);
int dmell_handler_recvf(int argc, char **argv, dmell_ctx_t *ctx);

#endif
