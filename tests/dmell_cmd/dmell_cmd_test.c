/**
 * @file dmell_cmd_test.c
 * @brief Unit tests for dmell command handling functions (dmell_cmd module)
 */
#include "dmod_test.h"
#include "dmell_cmd.h"
#include "dmell_redirect.h"
#include <string.h>

// Global variable to track command execution
static int g_last_argc = 0;
static char** g_last_argv = NULL;
static int g_return_value = 0;
static dmell_ctx_t g_ctx;
static dmell_argv_t parsed_argv;

void dmod_test_setup(void)
{
    g_last_argc = 0;
    g_last_argv = NULL;
    g_return_value = 0;
    memset(&g_ctx, 0, sizeof(g_ctx));
    memset(&parsed_argv, 0, sizeof(parsed_argv));
    dmell_set_default_handler(NULL);
}

void dmod_test_teardown(void)
{
    // Free parsed argv, if the step populated it
    for( int i = 0; i < parsed_argv.argc; i++ )
    {
        if( parsed_argv.argv != NULL && parsed_argv.argv[i] != NULL )
        {
            Dmod_Free( parsed_argv.argv[i] );
        }
    }
    Dmod_Free( parsed_argv.argv );

    for( size_t i = 0; i < parsed_argv.redirect_count; i++ )
    {
        Dmod_Free( parsed_argv.redirects[i].path );
    }
    Dmod_Free( parsed_argv.redirects );

    memset(&parsed_argv, 0, sizeof(parsed_argv));

    // Don't unregister commands: dmell_unregister_command's remove-from-middle
    // path is not exercised here - tests use unique command names instead so
    // leftover registrations never interfere with each other.
    dmell_set_default_handler(NULL);
}

// Test command handler
static int test_handler(int argc, char** argv, dmell_ctx_t* ctx)
{
    (void)ctx;
    g_last_argc = argc;
    g_last_argv = argv;
    return g_return_value;
}

// Another test command handler
static int test_handler2(int argc, char** argv, dmell_ctx_t* ctx)
{
    (void)argc;
    (void)argv;
    (void)ctx;
    return 42;
}

// Default command handler for testing
static int default_test_handler(int argc, char** argv, dmell_ctx_t* ctx)
{
    (void)ctx;
    g_last_argc = argc;
    g_last_argv = argv;
    return 99;
}

// ===============================================================
//                  Command Registration Tests
// ===============================================================

DMOD_TEST_STEP(register_command)
{
    dmell_cmd_t cmd = { .name = "test_cmd", .handler = test_handler };
    int result = dmell_register_command(&cmd);
    DMOD_TEST_EXPECT_EQ(result, 0);
}

DMOD_TEST_STEP(register_command_handler)
{
    int result = dmell_register_command_handler("test_cmd_2", test_handler);
    DMOD_TEST_EXPECT_EQ(result, 0);
}

DMOD_TEST_STEP(register_null_command)
{
    int result = dmell_register_command(NULL);
    DMOD_TEST_EXPECT(result < 0);
}

DMOD_TEST_STEP(register_command_null_name)
{
    dmell_cmd_t cmd = { .name = NULL, .handler = test_handler };
    int result = dmell_register_command(&cmd);
    DMOD_TEST_EXPECT(result < 0);
}

DMOD_TEST_STEP(register_command_null_handler)
{
    dmell_cmd_t cmd = { .name = "test_cmd_3", .handler = NULL };
    int result = dmell_register_command(&cmd);
    DMOD_TEST_EXPECT(result < 0);
}

DMOD_TEST_STEP(find_registered_command)
{
    dmell_register_command_handler("find_test_cmd", test_handler);

    const dmell_cmd_t* found = dmell_find_command("find_test_cmd");

    DMOD_TEST_EXPECT_NOT_NULL(found);
    DMOD_TEST_EXPECT(strcmp(found->name, "find_test_cmd") == 0);
    DMOD_TEST_EXPECT(found->handler == test_handler);
}

DMOD_TEST_STEP(find_non_existing_command)
{
    const dmell_cmd_t* found = dmell_find_command("nonexistent_xyz");
    DMOD_TEST_EXPECT_NULL(found);
}

DMOD_TEST_STEP(find_command_null_name)
{
    const dmell_cmd_t* found = dmell_find_command(NULL);
    DMOD_TEST_EXPECT_NULL(found);
}

DMOD_TEST_STEP(unregister_command)
{
    dmell_register_command_handler("unregister_test_cmd", test_handler);

    const dmell_cmd_t* cmd = dmell_find_command("unregister_test_cmd");
    DMOD_TEST_EXPECT_NOT_NULL(cmd);

    int result = dmell_unregister_command(cmd);

    DMOD_TEST_EXPECT_EQ(result, 0);
    DMOD_TEST_EXPECT_NULL(dmell_find_command("unregister_test_cmd"));
}

DMOD_TEST_STEP(unregister_null_command)
{
    int result = dmell_unregister_command(NULL);
    DMOD_TEST_EXPECT(result < 0);
}

DMOD_TEST_STEP(run_registered_command)
{
    dmell_register_command_handler("run_test_cmd", test_handler);
    g_return_value = 123;

    char* argv[] = { (char*)"run_test_cmd", (char*)"arg1", (char*)"arg2" };
    int result = dmell_run_command(&g_ctx, "run_test_cmd", 3, argv);

    DMOD_TEST_EXPECT_EQ(result, 123);
    DMOD_TEST_EXPECT_EQ(g_last_argc, 3);
    DMOD_TEST_EXPECT(g_last_argv == argv);
}

DMOD_TEST_STEP(run_non_existing_command_no_default)
{
    char* argv[] = { (char*)"nonexistent_abc" };
    int result = dmell_run_command(&g_ctx, "nonexistent_abc", 1, argv);
    DMOD_TEST_EXPECT(result < 0);
}

DMOD_TEST_STEP(run_non_existing_command_with_default)
{
    dmell_set_default_handler(default_test_handler);

    char* argv[] = { (char*)"nonexistent_def", (char*)"arg1" };
    int result = dmell_run_command(&g_ctx, "nonexistent_def", 2, argv);

    DMOD_TEST_EXPECT_EQ(result, 99);
    DMOD_TEST_EXPECT_EQ(g_last_argc, 2);
}

DMOD_TEST_STEP(run_command_null_name)
{
    char* argv[] = { (char*)"cmd" };
    int result = dmell_run_command(&g_ctx, NULL, 1, argv);
    DMOD_TEST_EXPECT(result < 0);
}

DMOD_TEST_STEP(set_default_handler)
{
    int result = dmell_set_default_handler(default_test_handler);
    DMOD_TEST_EXPECT_EQ(result, 0);
}

DMOD_TEST_STEP(register_multiple_commands)
{
    int result1 = dmell_register_command_handler("multi_test_cmd", test_handler);
    int result2 = dmell_register_command_handler("multi_test_cmd2", test_handler2);

    DMOD_TEST_EXPECT_EQ(result1, 0);
    DMOD_TEST_EXPECT_EQ(result2, 0);

    const dmell_cmd_t* cmd1 = dmell_find_command("multi_test_cmd");
    const dmell_cmd_t* cmd2 = dmell_find_command("multi_test_cmd2");

    DMOD_TEST_EXPECT_NOT_NULL(cmd1);
    DMOD_TEST_EXPECT_NOT_NULL(cmd2);
    DMOD_TEST_EXPECT(strcmp(cmd1->name, "multi_test_cmd") == 0);
    DMOD_TEST_EXPECT(strcmp(cmd2->name, "multi_test_cmd2") == 0);
}

// ===============================================================
//                  Command Parsing Tests
// ===============================================================

DMOD_TEST_STEP(parse_simple_command)
{
    const char* cmd = "echo hello";
    int result = dmell_parse_command(cmd, strlen(cmd), &parsed_argv);

    DMOD_TEST_EXPECT_EQ(result, 0);
    DMOD_TEST_EXPECT_EQ(parsed_argv.argc, 2);
    DMOD_TEST_EXPECT_NOT_NULL(parsed_argv.argv);
    DMOD_TEST_EXPECT(strcmp(parsed_argv.argv[0], "echo") == 0);
    DMOD_TEST_EXPECT(strcmp(parsed_argv.argv[1], "hello") == 0);
}

DMOD_TEST_STEP(parse_multiple_arguments)
{
    const char* cmd = "cmd arg1 arg2 arg3";
    int result = dmell_parse_command(cmd, strlen(cmd), &parsed_argv);

    DMOD_TEST_EXPECT_EQ(result, 0);
    DMOD_TEST_EXPECT_EQ(parsed_argv.argc, 4);
    DMOD_TEST_EXPECT(strcmp(parsed_argv.argv[0], "cmd") == 0);
    DMOD_TEST_EXPECT(strcmp(parsed_argv.argv[1], "arg1") == 0);
    DMOD_TEST_EXPECT(strcmp(parsed_argv.argv[2], "arg2") == 0);
    DMOD_TEST_EXPECT(strcmp(parsed_argv.argv[3], "arg3") == 0);
}

DMOD_TEST_STEP(parse_with_leading_whitespace)
{
    const char* cmd = "   cmd arg";
    int result = dmell_parse_command(cmd, strlen(cmd), &parsed_argv);

    DMOD_TEST_EXPECT_EQ(result, 0);
    DMOD_TEST_EXPECT_EQ(parsed_argv.argc, 2);
    DMOD_TEST_EXPECT(strcmp(parsed_argv.argv[0], "cmd") == 0);
}

DMOD_TEST_STEP(parse_with_extra_whitespace)
{
    const char* cmd = "cmd   arg1    arg2";
    int result = dmell_parse_command(cmd, strlen(cmd), &parsed_argv);

    DMOD_TEST_EXPECT_EQ(result, 0);
    DMOD_TEST_EXPECT_EQ(parsed_argv.argc, 3);
}

DMOD_TEST_STEP(parse_double_quoted_argument)
{
    const char* cmd = "echo \"hello world\"";
    int result = dmell_parse_command(cmd, strlen(cmd), &parsed_argv);

    DMOD_TEST_EXPECT_EQ(result, 0);
    DMOD_TEST_EXPECT_EQ(parsed_argv.argc, 2);
    DMOD_TEST_EXPECT(strcmp(parsed_argv.argv[1], "hello world") == 0);
}

DMOD_TEST_STEP(parse_single_quoted_argument)
{
    const char* cmd = "echo 'hello world'";
    int result = dmell_parse_command(cmd, strlen(cmd), &parsed_argv);

    DMOD_TEST_EXPECT_EQ(result, 0);
    DMOD_TEST_EXPECT_EQ(parsed_argv.argc, 2);
    DMOD_TEST_EXPECT(strcmp(parsed_argv.argv[1], "hello world") == 0);
}

DMOD_TEST_STEP(parse_null_command)
{
    int result = dmell_parse_command(NULL, 0, &parsed_argv);
    DMOD_TEST_EXPECT(result < 0);
}

DMOD_TEST_STEP(parse_empty_command)
{
    const char* cmd = "";
    int result = dmell_parse_command(cmd, 0, &parsed_argv);
    DMOD_TEST_EXPECT(result < 0);
}

DMOD_TEST_STEP(parse_null_output)
{
    const char* cmd = "echo hello";
    int result = dmell_parse_command(cmd, strlen(cmd), NULL);
    DMOD_TEST_EXPECT(result < 0);
}

DMOD_TEST_STEP(parse_single_command)
{
    const char* cmd = "pwd";
    int result = dmell_parse_command(cmd, strlen(cmd), &parsed_argv);

    DMOD_TEST_EXPECT_EQ(result, 0);
    DMOD_TEST_EXPECT_EQ(parsed_argv.argc, 1);
    DMOD_TEST_EXPECT(strcmp(parsed_argv.argv[0], "pwd") == 0);
}

DMOD_TEST_STEP(program_name_set)
{
    const char* cmd = "myprogram arg1";
    int result = dmell_parse_command(cmd, strlen(cmd), &parsed_argv);

    DMOD_TEST_EXPECT_EQ(result, 0);
    DMOD_TEST_EXPECT(strcmp(parsed_argv.program_name, "myprogram") == 0);
}

// ===============================================================
//                  Command String Execution Tests
// ===============================================================

static int testcmd_handler(int argc, char** argv, dmell_ctx_t* ctx)
{
    (void)ctx;
    g_last_argc = argc;
    g_last_argv = argv;
    return g_return_value;
}

DMOD_TEST_STEP(run_simple_command_string)
{
    dmell_register_command_handler("my_unique_testcmd", testcmd_handler);
    g_return_value = 0;
    const char* cmd = "my_unique_testcmd arg1 arg2";

    int result = dmell_run_command_string(&g_ctx, cmd, strlen(cmd));

    DMOD_TEST_EXPECT_EQ(result, 0);
    DMOD_TEST_EXPECT_EQ(g_last_argc, 3);
}

DMOD_TEST_STEP(run_null_command_string)
{
    int result = dmell_run_command_string(&g_ctx, NULL, 0);
    DMOD_TEST_EXPECT(result < 0);
}

DMOD_TEST_STEP(run_empty_command_string)
{
    const char* cmd = "";
    int result = dmell_run_command_string(&g_ctx, cmd, 0);
    DMOD_TEST_EXPECT(result < 0);
}

/**
 * This test binary links only the weak DMOD process API (no dmosi backend
 * provides a real Dmod_SetStreamFilePath), so requesting a redirect must fail
 * cleanly instead of silently running the command unredirected - see
 * dmell_redirect_apply_to_current_process()'s Dmod_IsFunctionConnected check.
 */
DMOD_TEST_STEP(run_string_with_redirect_fails_without_stream_backend)
{
    dmell_register_command_handler("my_unique_testcmd2", testcmd_handler);
    g_return_value = 0;
    const char* cmd = "my_unique_testcmd2 > /some/file";

    int result = dmell_run_command_string(&g_ctx, cmd, strlen(cmd));

    DMOD_TEST_EXPECT(result < 0);
    /* The command itself must not have run: the redirect could not be honored. */
    DMOD_TEST_EXPECT_EQ(g_last_argc, 0);
}

/**
 * A command with no redirection operators is unaffected by the redirect
 * apply/restore wrapper.
 */
DMOD_TEST_STEP(run_string_without_redirect_runs_normally)
{
    dmell_register_command_handler("my_unique_testcmd3", testcmd_handler);
    g_return_value = 0;
    const char* cmd = "my_unique_testcmd3 arg1";

    int result = dmell_run_command_string(&g_ctx, cmd, strlen(cmd));

    DMOD_TEST_EXPECT_EQ(result, 0);
    DMOD_TEST_EXPECT_EQ(g_last_argc, 2);
}

// ===============================================================
//                  Redirection Token Matching Tests
// ===============================================================

DMOD_TEST_STEP(matches_stdout_truncate)
{
    dmell_redirect_match_t m;
    DMOD_TEST_EXPECT(dmell_redirect_match_token(">", &m));
    DMOD_TEST_EXPECT_EQ(m.op_count, 1);
    DMOD_TEST_EXPECT_EQ(m.stream[0], DMELL_STREAM_STDOUT);
    DMOD_TEST_EXPECT_FALSE(m.is_dup);
    DMOD_TEST_EXPECT_FALSE(m.append);
    DMOD_TEST_EXPECT_TRUE(m.needs_target);
}

DMOD_TEST_STEP(matches_stdout_append_attached)
{
    dmell_redirect_match_t m;
    DMOD_TEST_EXPECT(dmell_redirect_match_token(">>out.txt", &m));
    DMOD_TEST_EXPECT_EQ(m.op_count, 1);
    DMOD_TEST_EXPECT_EQ(m.stream[0], DMELL_STREAM_STDOUT);
    DMOD_TEST_EXPECT_TRUE(m.append);
    DMOD_TEST_EXPECT_FALSE(m.needs_target);
    DMOD_TEST_EXPECT(strcmp(m.attached_target, "out.txt") == 0);
}

DMOD_TEST_STEP(matches_stderr_truncate)
{
    dmell_redirect_match_t m;
    DMOD_TEST_EXPECT(dmell_redirect_match_token("2>err.txt", &m));
    DMOD_TEST_EXPECT_EQ(m.op_count, 1);
    DMOD_TEST_EXPECT_EQ(m.stream[0], DMELL_STREAM_STDERR);
    DMOD_TEST_EXPECT_FALSE(m.append);
    DMOD_TEST_EXPECT(strcmp(m.attached_target, "err.txt") == 0);
}

DMOD_TEST_STEP(matches_stderr_append)
{
    dmell_redirect_match_t m;
    DMOD_TEST_EXPECT(dmell_redirect_match_token("2>>err.txt", &m));
    DMOD_TEST_EXPECT_EQ(m.stream[0], DMELL_STREAM_STDERR);
    DMOD_TEST_EXPECT_TRUE(m.append);
}

DMOD_TEST_STEP(matches_stdin)
{
    dmell_redirect_match_t m;
    DMOD_TEST_EXPECT(dmell_redirect_match_token("<input.txt", &m));
    DMOD_TEST_EXPECT_EQ(m.stream[0], DMELL_STREAM_STDIN);
    DMOD_TEST_EXPECT(strcmp(m.attached_target, "input.txt") == 0);
}

DMOD_TEST_STEP(matches_stderr_to_stdout_dup)
{
    dmell_redirect_match_t m;
    DMOD_TEST_EXPECT(dmell_redirect_match_token("2>&1", &m));
    DMOD_TEST_EXPECT_EQ(m.op_count, 1);
    DMOD_TEST_EXPECT_TRUE(m.is_dup);
    DMOD_TEST_EXPECT_EQ(m.stream[0], DMELL_STREAM_STDERR);
    DMOD_TEST_EXPECT_EQ(m.dup_source, DMELL_STREAM_STDOUT);
}

DMOD_TEST_STEP(matches_stdout_to_stderr_dup)
{
    dmell_redirect_match_t m;
    DMOD_TEST_EXPECT(dmell_redirect_match_token("1>&2", &m));
    DMOD_TEST_EXPECT_TRUE(m.is_dup);
    DMOD_TEST_EXPECT_EQ(m.stream[0], DMELL_STREAM_STDOUT);
    DMOD_TEST_EXPECT_EQ(m.dup_source, DMELL_STREAM_STDERR);
}

DMOD_TEST_STEP(matches_combined_truncate)
{
    dmell_redirect_match_t m;
    DMOD_TEST_EXPECT(dmell_redirect_match_token("&>all.txt", &m));
    DMOD_TEST_EXPECT_EQ(m.op_count, 2);
    DMOD_TEST_EXPECT_EQ(m.stream[0], DMELL_STREAM_STDOUT);
    DMOD_TEST_EXPECT_EQ(m.stream[1], DMELL_STREAM_STDERR);
    DMOD_TEST_EXPECT_FALSE(m.append);
    DMOD_TEST_EXPECT(strcmp(m.attached_target, "all.txt") == 0);
}

DMOD_TEST_STEP(matches_combined_append)
{
    dmell_redirect_match_t m;
    DMOD_TEST_EXPECT(dmell_redirect_match_token("&>>all.txt", &m));
    DMOD_TEST_EXPECT_EQ(m.op_count, 2);
    DMOD_TEST_EXPECT_TRUE(m.append);
}

DMOD_TEST_STEP(matches_stdlog_extension)
{
    dmell_redirect_match_t m;
    DMOD_TEST_EXPECT(dmell_redirect_match_token("3>log.txt", &m));
    DMOD_TEST_EXPECT_EQ(m.stream[0], DMELL_STREAM_STDLOG);
}

DMOD_TEST_STEP(rejects_ordinary_word)
{
    dmell_redirect_match_t m;
    DMOD_TEST_EXPECT_FALSE(dmell_redirect_match_token("hello", &m));
}

DMOD_TEST_STEP(rejects_plain_number)
{
    dmell_redirect_match_t m;
    DMOD_TEST_EXPECT_FALSE(dmell_redirect_match_token("123", &m));
}

// ===============================================================
//                  Redirection Parsing Tests (dmell_parse_command)
// ===============================================================

DMOD_TEST_STEP(parse_stdout_redirect_space_separated)
{
    const char* cmd = "echo hello > /some/file";
    int result = dmell_parse_command(cmd, strlen(cmd), &parsed_argv);

    DMOD_TEST_EXPECT_EQ(result, 0);
    DMOD_TEST_EXPECT_EQ(parsed_argv.argc, 2);
    DMOD_TEST_EXPECT(strcmp(parsed_argv.argv[0], "echo") == 0);
    DMOD_TEST_EXPECT(strcmp(parsed_argv.argv[1], "hello") == 0);

    DMOD_TEST_EXPECT_EQ((int)parsed_argv.redirect_count, 1);
    DMOD_TEST_EXPECT_EQ(parsed_argv.redirects[0].stream, DMELL_STREAM_STDOUT);
    DMOD_TEST_EXPECT_FALSE(parsed_argv.redirects[0].append);
    DMOD_TEST_EXPECT_NOT_NULL(parsed_argv.redirects[0].path);
    DMOD_TEST_EXPECT(strcmp(parsed_argv.redirects[0].path, "/some/file") == 0);
}

DMOD_TEST_STEP(parse_stdout_redirect_attached)
{
    const char* cmd = "echo hello >/some/file";
    int result = dmell_parse_command(cmd, strlen(cmd), &parsed_argv);

    DMOD_TEST_EXPECT_EQ(result, 0);
    DMOD_TEST_EXPECT_EQ(parsed_argv.argc, 2);
    DMOD_TEST_EXPECT_EQ((int)parsed_argv.redirect_count, 1);
    DMOD_TEST_EXPECT(strcmp(parsed_argv.redirects[0].path, "/some/file") == 0);
}

DMOD_TEST_STEP(parse_missing_redirect_target_fails)
{
    const char* cmd = "echo hello >";
    int result = dmell_parse_command(cmd, strlen(cmd), &parsed_argv);
    DMOD_TEST_EXPECT(result < 0);
}

DMOD_TEST_STEP(parse_no_redirect_leaves_empty_list)
{
    const char* cmd = "echo hello world";
    int result = dmell_parse_command(cmd, strlen(cmd), &parsed_argv);

    DMOD_TEST_EXPECT_EQ(result, 0);
    DMOD_TEST_EXPECT_EQ(parsed_argv.argc, 3);
    DMOD_TEST_EXPECT_EQ((int)parsed_argv.redirect_count, 0);
    DMOD_TEST_EXPECT_NULL(parsed_argv.redirects);
}

/**
 * `cmd >out.txt 2>&1` should redirect stdout to the file and duplicate
 * stderr onto stdout's *final* target, matching POSIX shell left-to-right
 * resolution order.
 */
DMOD_TEST_STEP(parse_combined_stdout_file_then_stderr_dup)
{
    const char* cmd = "mycmd arg1 >out.txt 2>&1";
    int result = dmell_parse_command(cmd, strlen(cmd), &parsed_argv);

    DMOD_TEST_EXPECT_EQ(result, 0);
    DMOD_TEST_EXPECT_EQ(parsed_argv.argc, 2);
    DMOD_TEST_EXPECT(strcmp(parsed_argv.argv[0], "mycmd") == 0);
    DMOD_TEST_EXPECT(strcmp(parsed_argv.argv[1], "arg1") == 0);

    DMOD_TEST_EXPECT_EQ((int)parsed_argv.redirect_count, 2);
    DMOD_TEST_EXPECT_EQ(parsed_argv.redirects[0].stream, DMELL_STREAM_STDOUT);
    DMOD_TEST_EXPECT(strcmp(parsed_argv.redirects[0].path, "out.txt") == 0);
    DMOD_TEST_EXPECT_TRUE(parsed_argv.redirects[1].is_dup);
    DMOD_TEST_EXPECT_EQ(parsed_argv.redirects[1].stream, DMELL_STREAM_STDERR);
    DMOD_TEST_EXPECT_EQ(parsed_argv.redirects[1].dup_source, DMELL_STREAM_STDOUT);
}

DMOD_TEST_STEP(parse_combined_redirect_expands_to_two_streams)
{
    const char* cmd = "mycmd &>all.txt";
    int result = dmell_parse_command(cmd, strlen(cmd), &parsed_argv);

    DMOD_TEST_EXPECT_EQ(result, 0);
    DMOD_TEST_EXPECT_EQ(parsed_argv.argc, 1);
    DMOD_TEST_EXPECT_EQ((int)parsed_argv.redirect_count, 2);
    DMOD_TEST_EXPECT_EQ(parsed_argv.redirects[0].stream, DMELL_STREAM_STDOUT);
    DMOD_TEST_EXPECT_EQ(parsed_argv.redirects[1].stream, DMELL_STREAM_STDERR);
    DMOD_TEST_EXPECT(strcmp(parsed_argv.redirects[0].path, "all.txt") == 0);
    DMOD_TEST_EXPECT(strcmp(parsed_argv.redirects[1].path, "all.txt") == 0);
    /* Each stream must own an independent copy of the path. */
    DMOD_TEST_EXPECT(parsed_argv.redirects[0].path != parsed_argv.redirects[1].path);
}

DMOD_TEST_STEP(parse_stdin_redirect)
{
    const char* cmd = "mycmd < input.txt";
    int result = dmell_parse_command(cmd, strlen(cmd), &parsed_argv);

    DMOD_TEST_EXPECT_EQ(result, 0);
    DMOD_TEST_EXPECT_EQ(parsed_argv.argc, 1);
    DMOD_TEST_EXPECT_EQ((int)parsed_argv.redirect_count, 1);
    DMOD_TEST_EXPECT_EQ(parsed_argv.redirects[0].stream, DMELL_STREAM_STDIN);
    DMOD_TEST_EXPECT(strcmp(parsed_argv.redirects[0].path, "input.txt") == 0);
}

// ===============================================================
//                  Apply/Restore Current Process Tests
// ===============================================================
//
// These only exercise the parts observable without a real DMOD process/stream
// backend (this test binary links just the weak DMOD process API - no dmosi).
// Full apply-then-restore behavior against a live backend is exercised by
// dmosi/dmosi-proc's own test suites.

DMOD_TEST_STEP(zero_redirects_is_a_no_op)
{
    dmell_redirect_backup_t backup;
    int result = dmell_redirect_apply_to_current_process(NULL, 0, &backup);

    DMOD_TEST_EXPECT_EQ(result, 0);
    DMOD_TEST_EXPECT_EQ((int)backup.touched_count, 0);

    /* Must be safe to call, and a no-op, even though nothing was applied. */
    dmell_redirect_restore_current_process(&backup);
}

DMOD_TEST_STEP(fails_cleanly_without_a_connected_backend)
{
    dmell_redirect_t redirect = {0};
    redirect.stream = DMELL_STREAM_STDOUT;
    redirect.path = Dmod_StrDup("/some/file");

    dmell_redirect_backup_t backup;
    int result = dmell_redirect_apply_to_current_process(&redirect, 1, &backup);

    DMOD_TEST_EXPECT(result < 0);
    DMOD_TEST_EXPECT_EQ((int)backup.touched_count, 0);

    Dmod_Free(redirect.path);
}
