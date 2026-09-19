/**
 * @file dmell_runtime_stubs.c
 * @brief Minimal freestanding-runtime stub required by libgcc's AEABI helpers.
 *
 * dmod links every module directly against libgcc.a to pull in the soft
 * ARM runtime helpers it needs - such as __aeabi_uldivmod, used by any
 * uint64_t division/modulo (e.g. dmell_core's uptime command, ps's
 * format_time()). libgcc's "Linux" flavor of the division-by-zero trap
 * (__aeabi_ldiv0, pulled in from _dvmd_lnx.o) calls raise(SIGFPE) to
 * emulate what glibc would do - but modules are built with -nostdlib, so
 * libc's raise() is never linked in, leaving it undefined. The divisors in
 * both cases are compile-time constants, so this trap is never actually
 * reachable; this stub exists only to satisfy the linker. Add this file's
 * path to any other module's sources that starts dividing/modulo-ing a
 * 64-bit integer and hits the same "undefined reference to raise" error.
 */
int raise(int sig)
{
    (void)sig;
    return 0;
}
