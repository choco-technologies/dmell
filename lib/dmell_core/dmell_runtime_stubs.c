/**
 * @file dmell_runtime_stubs.c
 * @brief Minimal freestanding-runtime stub required by libgcc's AEABI helpers.
 *
 * dmod links dmell_core directly against libgcc.a to pull in the soft ARM
 * runtime helpers it needs - such as __aeabi_uldivmod, used by
 * dmell_handlers.c's uptime command for 64-bit division/modulo on
 * Dmod_GetUptime()'s Dmod_Timestamp_t (uint64_t). libgcc's "Linux" flavor
 * of the division-by-zero trap (__aeabi_ldiv0, pulled in from _dvmd_lnx.o)
 * calls raise(SIGFPE) to emulate what glibc would do - but dmell_core is
 * built with -nostdlib, so libc's raise() is never linked in, leaving it
 * undefined. The uptime divisors are all constants, so this trap is never
 * actually reachable; this stub exists only to satisfy the linker.
 */
int raise(int sig)
{
    (void)sig;
    return 0;
}
