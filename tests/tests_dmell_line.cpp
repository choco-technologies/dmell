/**
 * @file tests_dmell_line.cpp
 * @brief Unit tests for dmell line execution functions
 */

#include <gtest/gtest.h>
#include <string.h>

extern "C" {
#include "dmell_line.h"
#include "dmell_cmd.h"
#include "dmod_sal.h"
}

// Global variable to track command execution
static int g_call_count = 0;
static int g_return_values[10] = {0};
static int g_return_index = 0;

// Reset global state
static void reset_line_globals()
{
    g_call_count = 0;
    g_return_index = 0;
    memset(g_return_values, 0, sizeof(g_return_values));
}

// Test command handler that returns values from array
static int counting_handler(int argc, char** argv)
{
    (void)argc;
    (void)argv;
    int ret = g_return_values[g_return_index];
    if (g_return_index < 9) g_return_index++;
    g_call_count++;
    return ret;
}

// Success handler
static int line_success_handler(int argc, char** argv)
{
    (void)argc;
    (void)argv;
    g_call_count++;
    return 0;
}

// Failure handler
static int line_failure_handler(int argc, char** argv)
{
    (void)argc;
    (void)argv;
    g_call_count++;
    return 1;
}

// ===============================================================
//                  Line Execution Tests
// ===============================================================

class DmellLineTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        reset_line_globals();
        // Use unique command names to avoid conflicts with other tests
        dmell_register_command_handler("line_cmd", counting_handler);
        dmell_register_command_handler("line_success", line_success_handler);
        dmell_register_command_handler("line_fail", line_failure_handler);
    }

    void TearDown() override
    {
        // Don't clean up due to bug in dmell_unregister_command
    }
};

/**
 * @brief Test running a simple line
 */
TEST_F(DmellLineTest, RunSimpleLine)
{
    g_return_values[0] = 0;
    const char* line = "line_cmd arg1";
    
    int result = dmell_run_line(line, strlen(line));
    
    EXPECT_EQ(result, 0);
    EXPECT_EQ(g_call_count, 1);
}

/**
 * @brief Test running null line
 */
TEST_F(DmellLineTest, RunNullLine)
{
    int result = dmell_run_line(nullptr, 0);
    
    EXPECT_LT(result, 0);
}

/**
 * @brief Test running empty line
 */
TEST_F(DmellLineTest, RunEmptyLine)
{
    int result = dmell_run_line("", 0);
    
    EXPECT_LT(result, 0);
}

/**
 * @brief Test running line with sequence separator (;)
 */
TEST_F(DmellLineTest, RunSequenceSeparator)
{
    g_return_values[0] = 0;
    g_return_values[1] = 0;
    const char* line = "line_cmd arg1; line_cmd arg2";
    
    int result = dmell_run_line(line, strlen(line));
    
    EXPECT_EQ(result, 0);
    EXPECT_EQ(g_call_count, 2);
}

/**
 * @brief Test running line with newline separator
 */
TEST_F(DmellLineTest, RunNewlineSeparator)
{
    g_return_values[0] = 0;
    g_return_values[1] = 0;
    const char* line = "line_cmd arg1\nline_cmd arg2";
    
    int result = dmell_run_line(line, strlen(line));
    
    EXPECT_EQ(result, 0);
    EXPECT_EQ(g_call_count, 2);
}

/**
 * @brief Test AND separator (&&) with both commands succeeding
 */
TEST_F(DmellLineTest, AndSeparatorBothSuccess)
{
    const char* line = "line_success && line_success";
    
    int result = dmell_run_line(line, strlen(line));
    
    EXPECT_EQ(result, 0);
    EXPECT_EQ(g_call_count, 2);
}

/**
 * @brief Test AND separator (&&) with first command failing
 */
TEST_F(DmellLineTest, AndSeparatorFirstFails)
{
    const char* line = "line_fail && line_success";
    
    int result = dmell_run_line(line, strlen(line));
    
    EXPECT_NE(result, 0);
    // Second command should NOT execute because first failed
    EXPECT_EQ(g_call_count, 1);
}

/**
 * @brief Test OR separator (||) with first command succeeding
 */
TEST_F(DmellLineTest, OrSeparatorFirstSuccess)
{
    const char* line = "line_success || line_fail";
    
    int result = dmell_run_line(line, strlen(line));
    
    EXPECT_EQ(result, 0);
    // Second command should NOT execute because first succeeded
    EXPECT_EQ(g_call_count, 1);
}

/**
 * @brief Test OR separator (||) with first command failing
 */
TEST_F(DmellLineTest, OrSeparatorFirstFails)
{
    const char* line = "line_fail || line_success";
    
    int result = dmell_run_line(line, strlen(line));
    
    EXPECT_EQ(result, 0);
    // Second command SHOULD execute because first failed
    EXPECT_EQ(g_call_count, 2);
}

/**
 * @brief Test multiple sequence commands
 */
TEST_F(DmellLineTest, MultipleSequenceCommands)
{
    g_return_values[0] = 0;
    g_return_values[1] = 0;
    g_return_values[2] = 0;
    const char* line = "line_cmd a; line_cmd b; line_cmd c";
    
    int result = dmell_run_line(line, strlen(line));
    
    EXPECT_EQ(result, 0);
    EXPECT_EQ(g_call_count, 3);
}

/**
 * @brief Test sequence ignores previous failure
 */
TEST_F(DmellLineTest, SequenceIgnoresPreviousFailure)
{
    const char* line = "line_fail; line_success";
    
    int result = dmell_run_line(line, strlen(line));
    
    // Result should be from the last command
    EXPECT_EQ(result, 0);
    // Both commands should execute
    EXPECT_EQ(g_call_count, 2);
}

/**
 * @brief Test complex combined separators
 */
TEST_F(DmellLineTest, ComplexCombinedSeparators)
{
    // line_success && line_fail || line_success
    // First: line_success (0) -> execute next due to &&
    // Second: line_fail (1) -> execute next due to ||
    // Third: line_success (0)
    const char* line = "line_success && line_fail || line_success";
    
    int result = dmell_run_line(line, strlen(line));
    
    EXPECT_EQ(result, 0);
    EXPECT_EQ(g_call_count, 3);
}

// ===============================================================
//                  Args Line Tests
// ===============================================================

class DmellArgsLineTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        reset_line_globals();
        dmell_register_command_handler("args_cmd", counting_handler);
    }

    void TearDown() override
    {
        // Don't clean up due to bug in dmell_unregister_command
    }
};

/**
 * @brief Test running args line
 */
TEST_F(DmellArgsLineTest, RunArgsLine)
{
    g_return_values[0] = 0;
    char* argv[] = { (char*)"args_cmd", (char*)"arg1", (char*)"arg2" };
    
    int result = dmell_run_args_line(3, argv);
    
    EXPECT_EQ(result, 0);
    EXPECT_EQ(g_call_count, 1);
}

/**
 * @brief Test running args line with null argv
 */
TEST_F(DmellArgsLineTest, RunArgsLineNullArgv)
{
    int result = dmell_run_args_line(1, nullptr);
    
    EXPECT_LT(result, 0);
}

/**
 * @brief Test running args line with zero argc
 */
TEST_F(DmellArgsLineTest, RunArgsLineZeroArgc)
{
    char* argv[] = { (char*)"args_cmd" };
    
    int result = dmell_run_args_line(0, argv);
    
    EXPECT_LT(result, 0);
}

/**
 * @brief Test running args line with negative argc
 */
TEST_F(DmellArgsLineTest, RunArgsLineNegativeArgc)
{
    char* argv[] = { (char*)"args_cmd" };
    
    int result = dmell_run_args_line(-1, argv);
    
    EXPECT_LT(result, 0);
}

/**
 * @brief Test running args line with separators in arguments
 */
TEST_F(DmellArgsLineTest, RunArgsLineWithSeparators)
{
    g_return_values[0] = 0;
    g_return_values[1] = 0;
    char* argv[] = { (char*)"args_cmd", (char*)"arg1;", (char*)"args_cmd", (char*)"arg2" };
    
    int result = dmell_run_args_line(4, argv);
    
    // This should treat the whole thing as one command line string
    // and parse separators correctly
    EXPECT_GE(g_call_count, 1);
}
