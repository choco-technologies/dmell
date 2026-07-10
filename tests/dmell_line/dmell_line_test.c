/**
 * @file dmell_line_test.c
 * @brief Unit tests for dmell line execution functions (dmell_line module)
 */
#include "dmod_test.h"
#include "dmell_line.h"
#include "dmell_cmd.h"
#include <string.h>

// Global variable to track command execution
static int g_call_count = 0;
static int g_return_values[10] = {0};
static int g_return_index = 0;
static dmell_ctx_t g_ctx;

void dmod_test_setup(void)
{
    g_call_count = 0;
    g_return_index = 0;
    memset(g_return_values, 0, sizeof(g_return_values));
    memset(&g_ctx, 0, sizeof(g_ctx));
}

void dmod_test_teardown(void)
{
    // Don't unregister commands - see dmell_cmd_test.c's comment on the same topic.
}

// Test command handler that returns values from array
static int counting_handler(int argc, char** argv, dmell_ctx_t* ctx)
{
    (void)argc;
    (void)argv;
    (void)ctx;
    int ret = g_return_values[g_return_index];
    if (g_return_index < 9) g_return_index++;
    g_call_count++;
    return ret;
}

// Success handler
static int line_success_handler(int argc, char** argv, dmell_ctx_t* ctx)
{
    (void)argc;
    (void)argv;
    (void)ctx;
    g_call_count++;
    return 0;
}

// Failure handler
static int line_failure_handler(int argc, char** argv, dmell_ctx_t* ctx)
{
    (void)argc;
    (void)argv;
    (void)ctx;
    g_call_count++;
    return 1;
}

// ===============================================================
//                  Line Execution Tests
// ===============================================================

DMOD_TEST_STEP(run_simple_line)
{
    dmell_register_command_handler("line_cmd", counting_handler);
    g_return_values[0] = 0;
    const char* line = "line_cmd arg1";

    int result = dmell_run_line(&g_ctx, line, strlen(line));

    DMOD_TEST_EXPECT_EQ(result, 0);
    DMOD_TEST_EXPECT_EQ(g_call_count, 1);
}

DMOD_TEST_STEP(run_null_line)
{
    int result = dmell_run_line(&g_ctx, NULL, 0);
    DMOD_TEST_EXPECT(result < 0);
}

DMOD_TEST_STEP(run_empty_line)
{
    int result = dmell_run_line(&g_ctx, "", 0);
    DMOD_TEST_EXPECT(result < 0);
}

DMOD_TEST_STEP(run_sequence_separator)
{
    dmell_register_command_handler("line_cmd", counting_handler);
    g_return_values[0] = 0;
    g_return_values[1] = 0;
    const char* line = "line_cmd arg1; line_cmd arg2";

    int result = dmell_run_line(&g_ctx, line, strlen(line));

    DMOD_TEST_EXPECT_EQ(result, 0);
    DMOD_TEST_EXPECT_EQ(g_call_count, 2);
}

DMOD_TEST_STEP(run_newline_separator)
{
    dmell_register_command_handler("line_cmd", counting_handler);
    g_return_values[0] = 0;
    g_return_values[1] = 0;
    const char* line = "line_cmd arg1\nline_cmd arg2";

    int result = dmell_run_line(&g_ctx, line, strlen(line));

    DMOD_TEST_EXPECT_EQ(result, 0);
    DMOD_TEST_EXPECT_EQ(g_call_count, 2);
}

DMOD_TEST_STEP(and_separator_both_success)
{
    dmell_register_command_handler("line_success", line_success_handler);
    const char* line = "line_success && line_success";

    int result = dmell_run_line(&g_ctx, line, strlen(line));

    DMOD_TEST_EXPECT_EQ(result, 0);
    DMOD_TEST_EXPECT_EQ(g_call_count, 2);
}

DMOD_TEST_STEP(and_separator_first_fails)
{
    dmell_register_command_handler("line_fail", line_failure_handler);
    dmell_register_command_handler("line_success", line_success_handler);
    const char* line = "line_fail && line_success";

    int result = dmell_run_line(&g_ctx, line, strlen(line));

    DMOD_TEST_EXPECT_NE(result, 0);
    // Second command should NOT execute because first failed
    DMOD_TEST_EXPECT_EQ(g_call_count, 1);
}

DMOD_TEST_STEP(or_separator_first_success)
{
    dmell_register_command_handler("line_success", line_success_handler);
    dmell_register_command_handler("line_fail", line_failure_handler);
    const char* line = "line_success || line_fail";

    int result = dmell_run_line(&g_ctx, line, strlen(line));

    DMOD_TEST_EXPECT_EQ(result, 0);
    // Second command should NOT execute because first succeeded
    DMOD_TEST_EXPECT_EQ(g_call_count, 1);
}

DMOD_TEST_STEP(or_separator_first_fails)
{
    dmell_register_command_handler("line_fail", line_failure_handler);
    dmell_register_command_handler("line_success", line_success_handler);
    const char* line = "line_fail || line_success";

    int result = dmell_run_line(&g_ctx, line, strlen(line));

    DMOD_TEST_EXPECT_EQ(result, 0);
    // Second command SHOULD execute because first failed
    DMOD_TEST_EXPECT_EQ(g_call_count, 2);
}

DMOD_TEST_STEP(multiple_sequence_commands)
{
    dmell_register_command_handler("line_cmd", counting_handler);
    g_return_values[0] = 0;
    g_return_values[1] = 0;
    g_return_values[2] = 0;
    const char* line = "line_cmd a; line_cmd b; line_cmd c";

    int result = dmell_run_line(&g_ctx, line, strlen(line));

    DMOD_TEST_EXPECT_EQ(result, 0);
    DMOD_TEST_EXPECT_EQ(g_call_count, 3);
}

DMOD_TEST_STEP(sequence_ignores_previous_failure)
{
    dmell_register_command_handler("line_fail", line_failure_handler);
    dmell_register_command_handler("line_success", line_success_handler);
    const char* line = "line_fail; line_success";

    int result = dmell_run_line(&g_ctx, line, strlen(line));

    // Result should be from the last command
    DMOD_TEST_EXPECT_EQ(result, 0);
    // Both commands should execute
    DMOD_TEST_EXPECT_EQ(g_call_count, 2);
}

DMOD_TEST_STEP(complex_combined_separators)
{
    dmell_register_command_handler("line_success", line_success_handler);
    dmell_register_command_handler("line_fail", line_failure_handler);
    // line_success && line_fail || line_success
    // First: line_success (0) -> execute next due to &&
    // Second: line_fail (1) -> execute next due to ||
    // Third: line_success (0)
    const char* line = "line_success && line_fail || line_success";

    int result = dmell_run_line(&g_ctx, line, strlen(line));

    DMOD_TEST_EXPECT_EQ(result, 0);
    DMOD_TEST_EXPECT_EQ(g_call_count, 3);
}

/**
 * A single trailing '&' must still be recognized as AND ('&&') when doubled,
 * not misparsed as two background separators - this guards the priority
 * ordering between is_and_separator() and the newer is_background_separator()
 * check.
 */
DMOD_TEST_STEP(double_ampersand_still_means_and)
{
    dmell_register_command_handler("line_success", line_success_handler);
    const char* line = "line_success && line_success";

    int result = dmell_run_line(&g_ctx, line, strlen(line));

    DMOD_TEST_EXPECT_EQ(result, 0);
    DMOD_TEST_EXPECT_EQ(g_call_count, 2);
}

/**
 * Background execution ('&') is only supported for external commands - a
 * registered built-in must be rejected outright, and never actually invoked,
 * since it has no independent execution context to background it into (see
 * dmell_bg.h).
 */
DMOD_TEST_STEP(background_separator_rejects_builtin)
{
    dmell_register_command_handler("line_success", line_success_handler);
    const char* line = "line_success &";

    int result = dmell_run_line(&g_ctx, line, strlen(line));

    DMOD_TEST_EXPECT(result < 0);
    DMOD_TEST_EXPECT_EQ(g_call_count, 0);
}

/**
 * After a backgrounded (rejected) segment, the rest of the line still runs -
 * background never blocks/aborts the remainder of the line, same as ';'.
 */
DMOD_TEST_STEP(background_separator_does_not_block_rest_of_line)
{
    dmell_register_command_handler("line_success", line_success_handler);
    dmell_register_command_handler("line_cmd", counting_handler);
    const char* line = "line_success & line_cmd arg1";
    g_return_values[0] = 0;

    dmell_run_line(&g_ctx, line, strlen(line));

    DMOD_TEST_EXPECT_EQ(g_call_count, 1);
}

// ===============================================================
//                  Args Line Tests
// ===============================================================

DMOD_TEST_STEP(run_args_line)
{
    dmell_register_command_handler("args_cmd", counting_handler);
    g_return_values[0] = 0;
    char* argv[] = { (char*)"args_cmd", (char*)"arg1", (char*)"arg2" };

    int result = dmell_run_args_line(&g_ctx, 3, argv);

    DMOD_TEST_EXPECT_EQ(result, 0);
    DMOD_TEST_EXPECT_EQ(g_call_count, 1);
}

DMOD_TEST_STEP(run_args_line_null_argv)
{
    int result = dmell_run_args_line(&g_ctx, 1, NULL);
    DMOD_TEST_EXPECT(result < 0);
}

DMOD_TEST_STEP(run_args_line_zero_argc)
{
    char* argv[] = { (char*)"args_cmd" };
    int result = dmell_run_args_line(&g_ctx, 0, argv);
    DMOD_TEST_EXPECT(result < 0);
}

DMOD_TEST_STEP(run_args_line_negative_argc)
{
    char* argv[] = { (char*)"args_cmd" };
    int result = dmell_run_args_line(&g_ctx, -1, argv);
    DMOD_TEST_EXPECT(result < 0);
}

DMOD_TEST_STEP(run_args_line_with_separators)
{
    dmell_register_command_handler("args_cmd", counting_handler);
    g_return_values[0] = 0;
    g_return_values[1] = 0;
    char* argv[] = { (char*)"args_cmd", (char*)"arg1;", (char*)"args_cmd", (char*)"arg2" };

    dmell_run_args_line(&g_ctx, 4, argv);

    // This should treat the whole thing as one command line string
    // and parse separators correctly
    DMOD_TEST_EXPECT(g_call_count >= 1);
}
