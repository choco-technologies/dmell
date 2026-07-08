#ifndef DMELL_REDIRECT_H
#define DMELL_REDIRECT_H

#include <stdbool.h>
#include <stddef.h>
#include <dmod.h>

/**
 * @file dmell_redirect.h
 * @brief Shell-style stream redirection support for dmell (>, >>, <, 2>&1, &>, ...).
 *
 * Every dmell command - built-in or external - ultimately runs somewhere inside
 * dmell's own process (built-ins directly, external DMOD modules either in-process
 * via Dmod_RunModule or in a freshly spawned child that inherits dmell's current
 * stream bindings). So redirection is implemented uniformly: temporarily rebind
 * dmell's own process streams for the duration of the command, then restore them
 * exactly, via dmell_redirect_apply_to_current_process() / _restore_current_process().
 *
 * "Restore exactly" matters: a stream that was unbound before must be cleared back
 * to unbound (not left pointing at the file we redirected to), and a stream that was
 * already bound to something else (e.g. a platform-specific log route) must be put
 * back to that, not just cleared - clearing it would silently switch it to whatever
 * a bare unbound stream falls back to, which is not necessarily the same target.
 */

/**
 * @brief One of the four well-known DMOD standard streams, indexed the same
 *        way shells number file descriptors (0=stdin, 1=stdout, 2=stderr).
 *        3 (stdlog) is a dmell-specific extension with no POSIX equivalent.
 */
typedef enum
{
    DMELL_STREAM_STDIN  = 0,
    DMELL_STREAM_STDOUT = 1,
    DMELL_STREAM_STDERR = 2,
    DMELL_STREAM_STDLOG = 3,
    DMELL_STREAM_COUNT
} dmell_stream_t;

/**
 * @brief One parsed redirection operator (e.g. '>', '2>>', '2>&1').
 *
 * Operators are kept in the order they were encountered on the command line:
 * resolving them left to right (like a real shell) is what makes constructs
 * such as `cmd 2>&1 >out.txt` and `cmd >out.txt 2>&1` behave differently.
 */
typedef struct
{
    dmell_stream_t stream;      /**< Stream this operator targets */
    char*          path;        /**< Owned target file path, or NULL when is_dup is true */
    bool           is_dup;      /**< True for 'N>&M' style fd-duplication operators */
    dmell_stream_t dup_source;  /**< Stream to duplicate from, valid only when is_dup */
    bool           append;      /**< Append instead of truncate, valid only when !is_dup */
} dmell_redirect_t;

/**
 * @brief Result of matching a single token against the redirection grammar.
 *
 * A single token can expand to up to two operators: '&>' / '&>>' apply to
 * both stdout and stderr at once.
 */
typedef struct
{
    bool           matched;         /**< True if the token is a redirection operator */
    int            op_count;        /**< Number of operators produced (1, or 2 for &>) */
    dmell_stream_t stream[2];       /**< Target stream(s) */
    bool           is_dup;          /**< True for 'N>&M' duplication (always op_count == 1) */
    dmell_stream_t dup_source;      /**< Valid only when is_dup */
    bool           append;          /**< Valid only when !is_dup */
    bool           needs_target;    /**< True if the target path is not fused to this token */
    const char*    attached_target; /**< Pointer into the token where a fused target begins */
} dmell_redirect_match_t;

/**
 * @brief Try to match a whitespace-delimited token against the redirection grammar.
 *
 * Recognizes: '<', '>', '>>', '2>', '2>>', '3>', '3>>', '&>', '&>>', and fd
 * duplication forms like '2>&1' / '1>&2'. The target file path may be fused to
 * the operator (e.g. ">file") or left for the caller to take from the next
 * token (out->needs_target is set, out->attached_target points at an empty string).
 *
 * @param token Candidate token, NUL-terminated
 * @param out   Match result, valid only when the return value is true
 * @return true if the token is a redirection operator
 */
extern bool dmell_redirect_match_token( const char* token, dmell_redirect_match_t* out );

/**
 * @brief Free a redirect array previously built while parsing a command.
 *
 * @param redirects Array to free (may be NULL)
 * @param count     Number of entries in the array
 */
extern void dmell_redirect_free( dmell_redirect_t* redirects, size_t count );

/**
 * @brief Resolve parsed redirections into DMOD stream redirection entries.
 *
 * Walks the operators left to right, maintaining the file each stream currently
 * targets so that 'N>&M' duplication and repeated redirects to the same stream
 * resolve the same way a POSIX shell would. Truncating operators (everything
 * except '>>' / '&>>') create/empty their target file immediately, matching
 * shell behavior of truncating even before the command is known to be runnable.
 *
 * @param redirects   Parsed redirection operators, in encountered order
 * @param count       Number of entries in @p redirects
 * @param out_entries Buffer for resolved entries, must hold at least DMELL_STREAM_COUNT items
 * @param out_count   Receives the number of entries written to out_entries
 * @return 0 on success, negative errno value if a target file could not be prepared
 */
extern int dmell_redirect_resolve( const dmell_redirect_t* redirects, size_t count,
                                    Dmod_StreamRedirection_t* out_entries, size_t* out_count );

/**
 * @brief Snapshot needed to restore dmell's own process streams after a temporary redirect.
 *
 * Populated by dmell_redirect_apply_to_current_process() and consumed by
 * dmell_redirect_restore_current_process(); not meant to be built by hand.
 */
typedef struct
{
    void*  touched[DMELL_STREAM_COUNT];                  /**< StdHandles this redirect changed */
    size_t touched_count;
    Dmod_StreamRedirection_t previous[DMELL_STREAM_COUNT]; /**< Prior bindings (owned Path), only for touched streams */
    size_t previous_count;
} dmell_redirect_backup_t;

/**
 * @brief Temporarily apply redirections to dmell's own current process.
 *
 * Snapshots whatever each targeted stream is currently bound to (so it can be restored
 * exactly, whether that was "unbound" or some other explicit binding), then applies the
 * requested redirections via Dmod_SetStreamFilePath. On failure, any streams already
 * changed before the failing one are rolled back automatically before returning.
 *
 * @param redirects  Parsed redirection operators to apply, in encountered order
 * @param count      Number of entries in @p redirects (0 is a valid, cheap no-op)
 * @param out_backup Filled in on success; pass to dmell_redirect_restore_current_process() afterwards
 * @return 0 on success, negative errno value on failure (nothing left applied in that case)
 */
extern int dmell_redirect_apply_to_current_process( const dmell_redirect_t* redirects, size_t count,
                                                      dmell_redirect_backup_t* out_backup );

/**
 * @brief Undo dmell_redirect_apply_to_current_process(), restoring prior stream bindings.
 *
 * Safe to call with a backup produced from a zero-count apply (no-op).
 *
 * @param backup Backup produced by a matching dmell_redirect_apply_to_current_process() call
 */
extern void dmell_redirect_restore_current_process( const dmell_redirect_backup_t* backup );

#endif // DMELL_REDIRECT_H
