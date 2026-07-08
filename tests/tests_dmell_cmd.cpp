/**
 * @file tests_dmell_cmd.cpp
 * @brief Unit tests for dmell command handling functions
 */

#include <gtest/gtest.h>
#include <string.h>

extern "C" {
#include "dmell_cmd.h"
#include "dmell_redirect.h"
#include "dmod_sal.h"
}

// Global variable to track command execution
static int g_last_argc = 0;
static char** g_last_argv = nullptr;
static int g_return_value = 0;

// Reset global state before each test
static void reset_globals()
{
    g_last_argc = 0;
    g_last_argv = nullptr;
    g_return_value = 0;
}

// Test command handler
static int test_handler(int argc, char** argv)
{
    g_last_argc = argc;
    g_last_argv = argv;
    return g_return_value;
}

// Another test command handler
static int test_handler2(int argc, char** argv)
{
    (void)argc;
    (void)argv;
    return 42;
}

// Default command handler for testing
static int default_test_handler(int argc, char** argv)
{
    g_last_argc = argc;
    g_last_argv = argv;
    return 99;
}

// ===============================================================
//                  Command Registration Tests
// ===============================================================

class DmellCmdTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        reset_globals();
        // Note: We don't clean up leftover commands because the dmell_unregister_command 
        // has a bug that causes memory corruption when removing from the middle of the array.
        // Each test should use unique command names to avoid conflicts.
        dmell_set_default_handler(nullptr);
    }

    void TearDown() override
    {
        // Don't clean up commands due to bug in dmell_unregister_command
        // that causes double-free when removing multiple commands.
        dmell_set_default_handler(nullptr);
    }
};

/**
 * @brief Test registering a command
 */
TEST_F(DmellCmdTest, RegisterCommand)
{
    dmell_cmd_t cmd = {
        .name = "test_cmd",
        .handler = test_handler
    };
    
    int result = dmell_register_command(&cmd);
    
    EXPECT_EQ(result, 0);
}

/**
 * @brief Test registering a command using handler function
 */
TEST_F(DmellCmdTest, RegisterCommandHandler)
{
    int result = dmell_register_command_handler("test_cmd", test_handler);
    
    EXPECT_EQ(result, 0);
}

/**
 * @brief Test registering null command
 */
TEST_F(DmellCmdTest, RegisterNullCommand)
{
    int result = dmell_register_command(nullptr);
    
    EXPECT_LT(result, 0);
}

/**
 * @brief Test registering command with null name
 */
TEST_F(DmellCmdTest, RegisterCommandNullName)
{
    dmell_cmd_t cmd = {
        .name = nullptr,
        .handler = test_handler
    };
    
    int result = dmell_register_command(&cmd);
    
    EXPECT_LT(result, 0);
}

/**
 * @brief Test registering command with null handler
 */
TEST_F(DmellCmdTest, RegisterCommandNullHandler)
{
    dmell_cmd_t cmd = {
        .name = "test_cmd",
        .handler = nullptr
    };
    
    int result = dmell_register_command(&cmd);
    
    EXPECT_LT(result, 0);
}

/**
 * @brief Test finding a registered command
 */
TEST_F(DmellCmdTest, FindRegisteredCommand)
{
    dmell_register_command_handler("test_cmd", test_handler);
    
    const dmell_cmd_t* found = dmell_find_command("test_cmd");
    
    ASSERT_NE(found, nullptr);
    EXPECT_STREQ(found->name, "test_cmd");
    EXPECT_EQ(found->handler, test_handler);
}

/**
 * @brief Test finding non-existing command
 */
TEST_F(DmellCmdTest, FindNonExistingCommand)
{
    const dmell_cmd_t* found = dmell_find_command("nonexistent");
    
    EXPECT_EQ(found, nullptr);
}

/**
 * @brief Test finding command with null name
 */
TEST_F(DmellCmdTest, FindCommandNullName)
{
    const dmell_cmd_t* found = dmell_find_command(nullptr);
    
    EXPECT_EQ(found, nullptr);
}

/**
 * @brief Test unregistering a command
 */
TEST_F(DmellCmdTest, UnregisterCommand)
{
    // Use unique name to avoid conflicts with other tests
    dmell_register_command_handler("unregister_test_cmd", test_handler);
    
    const dmell_cmd_t* cmd = dmell_find_command("unregister_test_cmd");
    ASSERT_NE(cmd, nullptr);
    
    int result = dmell_unregister_command(cmd);
    
    EXPECT_EQ(result, 0);
    EXPECT_EQ(dmell_find_command("unregister_test_cmd"), nullptr);
}

/**
 * @brief Test unregistering null command
 */
TEST_F(DmellCmdTest, UnregisterNullCommand)
{
    int result = dmell_unregister_command(nullptr);
    
    EXPECT_LT(result, 0);
}

/**
 * @brief Test running a registered command
 */
TEST_F(DmellCmdTest, RunRegisteredCommand)
{
    dmell_register_command_handler("test_cmd", test_handler);
    g_return_value = 123;
    
    char* argv[] = { (char*)"test_cmd", (char*)"arg1", (char*)"arg2" };
    int result = dmell_run_command("test_cmd", 3, argv);
    
    EXPECT_EQ(result, 123);
    EXPECT_EQ(g_last_argc, 3);
    EXPECT_EQ(g_last_argv, argv);
}

/**
 * @brief Test running non-existing command without default handler
 */
TEST_F(DmellCmdTest, RunNonExistingCommandNoDefault)
{
    char* argv[] = { (char*)"nonexistent" };
    int result = dmell_run_command("nonexistent", 1, argv);
    
    EXPECT_LT(result, 0);
}

/**
 * @brief Test running non-existing command with default handler
 */
TEST_F(DmellCmdTest, RunNonExistingCommandWithDefault)
{
    dmell_set_default_handler(default_test_handler);
    
    char* argv[] = { (char*)"nonexistent", (char*)"arg1" };
    int result = dmell_run_command("nonexistent", 2, argv);
    
    EXPECT_EQ(result, 99);
    EXPECT_EQ(g_last_argc, 2);
}

/**
 * @brief Test running command with null name
 */
TEST_F(DmellCmdTest, RunCommandNullName)
{
    char* argv[] = { (char*)"cmd" };
    int result = dmell_run_command(nullptr, 1, argv);
    
    EXPECT_LT(result, 0);
}

/**
 * @brief Test setting default handler
 */
TEST_F(DmellCmdTest, SetDefaultHandler)
{
    int result = dmell_set_default_handler(default_test_handler);
    
    EXPECT_EQ(result, 0);
}

/**
 * @brief Test registering multiple commands
 */
TEST_F(DmellCmdTest, RegisterMultipleCommands)
{
    int result1 = dmell_register_command_handler("test_cmd", test_handler);
    int result2 = dmell_register_command_handler("test_cmd2", test_handler2);
    
    EXPECT_EQ(result1, 0);
    EXPECT_EQ(result2, 0);
    
    const dmell_cmd_t* cmd1 = dmell_find_command("test_cmd");
    const dmell_cmd_t* cmd2 = dmell_find_command("test_cmd2");
    
    ASSERT_NE(cmd1, nullptr);
    ASSERT_NE(cmd2, nullptr);
    EXPECT_STREQ(cmd1->name, "test_cmd");
    EXPECT_STREQ(cmd2->name, "test_cmd2");
}

// ===============================================================
//                  Command Parsing Tests
// ===============================================================

class DmellCmdParseTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        memset(&parsed_argv, 0, sizeof(parsed_argv));
    }

    void TearDown() override
    {
        // Free parsed argv
        for (int i = 0; i < parsed_argv.argc; i++)
        {
            if (parsed_argv.argv != nullptr && parsed_argv.argv[i] != nullptr)
            {
                Dmod_Free(parsed_argv.argv[i]);
            }
        }
        if (parsed_argv.argv != nullptr)
        {
            Dmod_Free(parsed_argv.argv);
        }
    }

    dmell_argv_t parsed_argv;
};

/**
 * @brief Test parsing simple command
 */
TEST_F(DmellCmdParseTest, ParseSimpleCommand)
{
    const char* cmd = "echo hello";
    
    int result = dmell_parse_command(cmd, strlen(cmd), &parsed_argv);
    
    EXPECT_EQ(result, 0);
    EXPECT_EQ(parsed_argv.argc, 2);
    ASSERT_NE(parsed_argv.argv, nullptr);
    EXPECT_STREQ(parsed_argv.argv[0], "echo");
    EXPECT_STREQ(parsed_argv.argv[1], "hello");
}

/**
 * @brief Test parsing command with multiple arguments
 */
TEST_F(DmellCmdParseTest, ParseMultipleArguments)
{
    const char* cmd = "cmd arg1 arg2 arg3";
    
    int result = dmell_parse_command(cmd, strlen(cmd), &parsed_argv);
    
    EXPECT_EQ(result, 0);
    EXPECT_EQ(parsed_argv.argc, 4);
    EXPECT_STREQ(parsed_argv.argv[0], "cmd");
    EXPECT_STREQ(parsed_argv.argv[1], "arg1");
    EXPECT_STREQ(parsed_argv.argv[2], "arg2");
    EXPECT_STREQ(parsed_argv.argv[3], "arg3");
}

/**
 * @brief Test parsing command with leading whitespace
 */
TEST_F(DmellCmdParseTest, ParseWithLeadingWhitespace)
{
    const char* cmd = "   cmd arg";
    
    int result = dmell_parse_command(cmd, strlen(cmd), &parsed_argv);
    
    EXPECT_EQ(result, 0);
    EXPECT_EQ(parsed_argv.argc, 2);
    EXPECT_STREQ(parsed_argv.argv[0], "cmd");
}

/**
 * @brief Test parsing command with extra whitespace
 */
TEST_F(DmellCmdParseTest, ParseWithExtraWhitespace)
{
    const char* cmd = "cmd   arg1    arg2";
    
    int result = dmell_parse_command(cmd, strlen(cmd), &parsed_argv);
    
    EXPECT_EQ(result, 0);
    EXPECT_EQ(parsed_argv.argc, 3);
}

/**
 * @brief Test parsing command with double quoted argument
 */
TEST_F(DmellCmdParseTest, ParseDoubleQuotedArgument)
{
    const char* cmd = "echo \"hello world\"";
    
    int result = dmell_parse_command(cmd, strlen(cmd), &parsed_argv);
    
    EXPECT_EQ(result, 0);
    EXPECT_EQ(parsed_argv.argc, 2);
    EXPECT_STREQ(parsed_argv.argv[1], "hello world");
}

/**
 * @brief Test parsing command with single quoted argument
 */
TEST_F(DmellCmdParseTest, ParseSingleQuotedArgument)
{
    const char* cmd = "echo 'hello world'";
    
    int result = dmell_parse_command(cmd, strlen(cmd), &parsed_argv);
    
    EXPECT_EQ(result, 0);
    EXPECT_EQ(parsed_argv.argc, 2);
    EXPECT_STREQ(parsed_argv.argv[1], "hello world");
}

/**
 * @brief Test parsing null command string
 */
TEST_F(DmellCmdParseTest, ParseNullCommand)
{
    int result = dmell_parse_command(nullptr, 0, &parsed_argv);
    
    EXPECT_LT(result, 0);
}

/**
 * @brief Test parsing empty command string
 */
TEST_F(DmellCmdParseTest, ParseEmptyCommand)
{
    const char* cmd = "";
    
    int result = dmell_parse_command(cmd, 0, &parsed_argv);
    
    EXPECT_LT(result, 0);
}

/**
 * @brief Test parsing command with null output
 */
TEST_F(DmellCmdParseTest, ParseNullOutput)
{
    const char* cmd = "echo hello";
    
    int result = dmell_parse_command(cmd, strlen(cmd), nullptr);
    
    EXPECT_LT(result, 0);
}

/**
 * @brief Test parsing single command without arguments
 */
TEST_F(DmellCmdParseTest, ParseSingleCommand)
{
    const char* cmd = "pwd";
    
    int result = dmell_parse_command(cmd, strlen(cmd), &parsed_argv);
    
    EXPECT_EQ(result, 0);
    EXPECT_EQ(parsed_argv.argc, 1);
    EXPECT_STREQ(parsed_argv.argv[0], "pwd");
}

/**
 * @brief Test that program_name is set correctly
 */
TEST_F(DmellCmdParseTest, ProgramNameSet)
{
    const char* cmd = "myprogram arg1";
    
    int result = dmell_parse_command(cmd, strlen(cmd), &parsed_argv);
    
    EXPECT_EQ(result, 0);
    EXPECT_STREQ(parsed_argv.program_name, "myprogram");
}

// ===============================================================
//                  Command String Execution Tests
// ===============================================================

// Store handler for use in tests
static int testcmd_handler(int argc, char** argv)
{
    g_last_argc = argc;
    g_last_argv = argv;
    return g_return_value;
}

class DmellCmdRunStringTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        reset_globals();
        // Register our test command - use a unique name to avoid conflicts
        dmell_register_command_handler("my_unique_testcmd", testcmd_handler);
    }

    void TearDown() override
    {
        // We don't clean up because the cleanup function has a bug
        // that causes double-free issues. This is acceptable for tests
        // as long as we use unique command names.
        dmell_set_default_handler(nullptr);
    }
};

/**
 * @brief Test running command string
 */
TEST_F(DmellCmdRunStringTest, RunSimpleCommandString)
{
    g_return_value = 0;
    const char* cmd = "my_unique_testcmd arg1 arg2";
    
    int result = dmell_run_command_string(cmd, strlen(cmd));
    
    EXPECT_EQ(result, 0);
    EXPECT_EQ(g_last_argc, 3);
}

/**
 * @brief Test running null command string
 */
TEST_F(DmellCmdRunStringTest, RunNullCommandString)
{
    int result = dmell_run_command_string(nullptr, 0);
    
    EXPECT_LT(result, 0);
}

/**
 * @brief Test running empty command string
 */
TEST_F(DmellCmdRunStringTest, RunEmptyCommandString)
{
    const char* cmd = "";

    int result = dmell_run_command_string(cmd, 0);

    EXPECT_LT(result, 0);
}

/**
 * @brief This test binary links only the weak DMOD process API (no dmosi backend
 * provides a real Dmod_SetStreamFilePath), so requesting a redirect must fail
 * cleanly instead of silently running the command unredirected - see
 * dmell_redirect_apply_to_current_process()'s Dmod_IsFunctionConnected check.
 */
TEST_F(DmellCmdRunStringTest, RunStringWithRedirectFailsWithoutStreamBackend)
{
    g_return_value = 0;
    const char* cmd = "my_unique_testcmd > /some/file";

    int result = dmell_run_command_string(cmd, strlen(cmd));

    EXPECT_LT(result, 0);
    /* The command itself must not have run: the redirect could not be honored. */
    EXPECT_EQ(g_last_argc, 0);
}

/**
 * @brief A command with no redirection operators is unaffected by the
 * redirect apply/restore wrapper.
 */
TEST_F(DmellCmdRunStringTest, RunStringWithoutRedirectRunsNormally)
{
    g_return_value = 0;
    const char* cmd = "my_unique_testcmd arg1";

    int result = dmell_run_command_string(cmd, strlen(cmd));

    EXPECT_EQ(result, 0);
    EXPECT_EQ(g_last_argc, 2);
}

// ===============================================================
//                  Redirection Token Matching Tests
// ===============================================================

class DmellRedirectMatchTest : public ::testing::Test
{
};

TEST_F(DmellRedirectMatchTest, MatchesStdoutTruncate)
{
    dmell_redirect_match_t m;
    ASSERT_TRUE(dmell_redirect_match_token(">", &m));
    EXPECT_EQ(m.op_count, 1);
    EXPECT_EQ(m.stream[0], DMELL_STREAM_STDOUT);
    EXPECT_FALSE(m.is_dup);
    EXPECT_FALSE(m.append);
    EXPECT_TRUE(m.needs_target);
}

TEST_F(DmellRedirectMatchTest, MatchesStdoutAppendAttached)
{
    dmell_redirect_match_t m;
    ASSERT_TRUE(dmell_redirect_match_token(">>out.txt", &m));
    EXPECT_EQ(m.op_count, 1);
    EXPECT_EQ(m.stream[0], DMELL_STREAM_STDOUT);
    EXPECT_TRUE(m.append);
    EXPECT_FALSE(m.needs_target);
    EXPECT_STREQ(m.attached_target, "out.txt");
}

TEST_F(DmellRedirectMatchTest, MatchesStderrTruncate)
{
    dmell_redirect_match_t m;
    ASSERT_TRUE(dmell_redirect_match_token("2>err.txt", &m));
    EXPECT_EQ(m.op_count, 1);
    EXPECT_EQ(m.stream[0], DMELL_STREAM_STDERR);
    EXPECT_FALSE(m.append);
    EXPECT_STREQ(m.attached_target, "err.txt");
}

TEST_F(DmellRedirectMatchTest, MatchesStderrAppend)
{
    dmell_redirect_match_t m;
    ASSERT_TRUE(dmell_redirect_match_token("2>>err.txt", &m));
    EXPECT_EQ(m.stream[0], DMELL_STREAM_STDERR);
    EXPECT_TRUE(m.append);
}

TEST_F(DmellRedirectMatchTest, MatchesStdin)
{
    dmell_redirect_match_t m;
    ASSERT_TRUE(dmell_redirect_match_token("<input.txt", &m));
    EXPECT_EQ(m.stream[0], DMELL_STREAM_STDIN);
    EXPECT_STREQ(m.attached_target, "input.txt");
}

TEST_F(DmellRedirectMatchTest, MatchesStderrToStdoutDup)
{
    dmell_redirect_match_t m;
    ASSERT_TRUE(dmell_redirect_match_token("2>&1", &m));
    EXPECT_EQ(m.op_count, 1);
    EXPECT_TRUE(m.is_dup);
    EXPECT_EQ(m.stream[0], DMELL_STREAM_STDERR);
    EXPECT_EQ(m.dup_source, DMELL_STREAM_STDOUT);
}

TEST_F(DmellRedirectMatchTest, MatchesStdoutToStderrDup)
{
    dmell_redirect_match_t m;
    ASSERT_TRUE(dmell_redirect_match_token("1>&2", &m));
    EXPECT_TRUE(m.is_dup);
    EXPECT_EQ(m.stream[0], DMELL_STREAM_STDOUT);
    EXPECT_EQ(m.dup_source, DMELL_STREAM_STDERR);
}

TEST_F(DmellRedirectMatchTest, MatchesCombinedTruncate)
{
    dmell_redirect_match_t m;
    ASSERT_TRUE(dmell_redirect_match_token("&>all.txt", &m));
    EXPECT_EQ(m.op_count, 2);
    EXPECT_EQ(m.stream[0], DMELL_STREAM_STDOUT);
    EXPECT_EQ(m.stream[1], DMELL_STREAM_STDERR);
    EXPECT_FALSE(m.append);
    EXPECT_STREQ(m.attached_target, "all.txt");
}

TEST_F(DmellRedirectMatchTest, MatchesCombinedAppend)
{
    dmell_redirect_match_t m;
    ASSERT_TRUE(dmell_redirect_match_token("&>>all.txt", &m));
    EXPECT_EQ(m.op_count, 2);
    EXPECT_TRUE(m.append);
}

TEST_F(DmellRedirectMatchTest, MatchesStdlogExtension)
{
    dmell_redirect_match_t m;
    ASSERT_TRUE(dmell_redirect_match_token("3>log.txt", &m));
    EXPECT_EQ(m.stream[0], DMELL_STREAM_STDLOG);
}

TEST_F(DmellRedirectMatchTest, RejectsOrdinaryWord)
{
    dmell_redirect_match_t m;
    EXPECT_FALSE(dmell_redirect_match_token("hello", &m));
}

TEST_F(DmellRedirectMatchTest, RejectsPlainNumber)
{
    dmell_redirect_match_t m;
    EXPECT_FALSE(dmell_redirect_match_token("123", &m));
}

// ===============================================================
//                  Redirection Parsing Tests (dmell_parse_command)
// ===============================================================

class DmellCmdRedirectParseTest : public ::testing::Test
{
protected:
    dmell_argv_t parsed_argv;

    void SetUp() override
    {
        memset(&parsed_argv, 0, sizeof(parsed_argv));
    }

    void TearDown() override
    {
        for (int i = 0; i < parsed_argv.argc; i++)
        {
            if (parsed_argv.argv != nullptr && parsed_argv.argv[i] != nullptr)
            {
                Dmod_Free(parsed_argv.argv[i]);
            }
        }
        Dmod_Free(parsed_argv.argv);

        for (size_t i = 0; i < parsed_argv.redirect_count; i++)
        {
            Dmod_Free(parsed_argv.redirects[i].path);
        }
        Dmod_Free(parsed_argv.redirects);
    }
};

TEST_F(DmellCmdRedirectParseTest, ParseStdoutRedirectSpaceSeparated)
{
    const char* cmd = "echo hello > /some/file";

    int result = dmell_parse_command(cmd, strlen(cmd), &parsed_argv);

    EXPECT_EQ(result, 0);
    ASSERT_EQ(parsed_argv.argc, 2);
    EXPECT_STREQ(parsed_argv.argv[0], "echo");
    EXPECT_STREQ(parsed_argv.argv[1], "hello");

    ASSERT_EQ(parsed_argv.redirect_count, 1u);
    EXPECT_EQ(parsed_argv.redirects[0].stream, DMELL_STREAM_STDOUT);
    EXPECT_FALSE(parsed_argv.redirects[0].append);
    ASSERT_NE(parsed_argv.redirects[0].path, nullptr);
    EXPECT_STREQ(parsed_argv.redirects[0].path, "/some/file");
}

TEST_F(DmellCmdRedirectParseTest, ParseStdoutRedirectAttached)
{
    const char* cmd = "echo hello >/some/file";

    int result = dmell_parse_command(cmd, strlen(cmd), &parsed_argv);

    EXPECT_EQ(result, 0);
    ASSERT_EQ(parsed_argv.argc, 2);
    ASSERT_EQ(parsed_argv.redirect_count, 1u);
    EXPECT_STREQ(parsed_argv.redirects[0].path, "/some/file");
}

TEST_F(DmellCmdRedirectParseTest, ParseMissingRedirectTargetFails)
{
    const char* cmd = "echo hello >";

    int result = dmell_parse_command(cmd, strlen(cmd), &parsed_argv);

    EXPECT_LT(result, 0);
}

TEST_F(DmellCmdRedirectParseTest, ParseNoRedirectLeavesEmptyList)
{
    const char* cmd = "echo hello world";

    int result = dmell_parse_command(cmd, strlen(cmd), &parsed_argv);

    EXPECT_EQ(result, 0);
    EXPECT_EQ(parsed_argv.argc, 3);
    EXPECT_EQ(parsed_argv.redirect_count, 0u);
    EXPECT_EQ(parsed_argv.redirects, nullptr);
}

/**
 * @brief `cmd >out.txt 2>&1` should redirect stdout to the file and duplicate
 * stderr onto stdout's *final* target, matching POSIX shell left-to-right
 * resolution order.
 */
TEST_F(DmellCmdRedirectParseTest, ParseCombinedStdoutFileThenStderrDup)
{
    const char* cmd = "mycmd arg1 >out.txt 2>&1";

    int result = dmell_parse_command(cmd, strlen(cmd), &parsed_argv);

    EXPECT_EQ(result, 0);
    ASSERT_EQ(parsed_argv.argc, 2);
    EXPECT_STREQ(parsed_argv.argv[0], "mycmd");
    EXPECT_STREQ(parsed_argv.argv[1], "arg1");

    ASSERT_EQ(parsed_argv.redirect_count, 2u);
    EXPECT_EQ(parsed_argv.redirects[0].stream, DMELL_STREAM_STDOUT);
    EXPECT_STREQ(parsed_argv.redirects[0].path, "out.txt");
    EXPECT_TRUE(parsed_argv.redirects[1].is_dup);
    EXPECT_EQ(parsed_argv.redirects[1].stream, DMELL_STREAM_STDERR);
    EXPECT_EQ(parsed_argv.redirects[1].dup_source, DMELL_STREAM_STDOUT);
}

TEST_F(DmellCmdRedirectParseTest, ParseCombinedRedirectExpandsToTwoStreams)
{
    const char* cmd = "mycmd &>all.txt";

    int result = dmell_parse_command(cmd, strlen(cmd), &parsed_argv);

    EXPECT_EQ(result, 0);
    ASSERT_EQ(parsed_argv.argc, 1);
    ASSERT_EQ(parsed_argv.redirect_count, 2u);
    EXPECT_EQ(parsed_argv.redirects[0].stream, DMELL_STREAM_STDOUT);
    EXPECT_EQ(parsed_argv.redirects[1].stream, DMELL_STREAM_STDERR);
    EXPECT_STREQ(parsed_argv.redirects[0].path, "all.txt");
    EXPECT_STREQ(parsed_argv.redirects[1].path, "all.txt");
    /* Each stream must own an independent copy of the path. */
    EXPECT_NE(parsed_argv.redirects[0].path, parsed_argv.redirects[1].path);
}

TEST_F(DmellCmdRedirectParseTest, ParseStdinRedirect)
{
    const char* cmd = "mycmd < input.txt";

    int result = dmell_parse_command(cmd, strlen(cmd), &parsed_argv);

    EXPECT_EQ(result, 0);
    ASSERT_EQ(parsed_argv.argc, 1);
    ASSERT_EQ(parsed_argv.redirect_count, 1u);
    EXPECT_EQ(parsed_argv.redirects[0].stream, DMELL_STREAM_STDIN);
    EXPECT_STREQ(parsed_argv.redirects[0].path, "input.txt");
}

// ===============================================================
//                  Apply/Restore Current Process Tests
// ===============================================================
//
// These only exercise the parts observable without a real DMOD process/stream
// backend (this test binary links just the weak DMOD process API - no dmosi).
// Full apply-then-restore behavior against a live backend is exercised by
// dmosi/dmosi-proc's own test suites.

class DmellRedirectApplyTest : public ::testing::Test
{
};

TEST_F(DmellRedirectApplyTest, ZeroRedirectsIsANoOp)
{
    dmell_redirect_backup_t backup;
    int result = dmell_redirect_apply_to_current_process(nullptr, 0, &backup);

    EXPECT_EQ(result, 0);
    EXPECT_EQ(backup.touched_count, 0u);

    /* Must be safe to call, and a no-op, even though nothing was applied. */
    dmell_redirect_restore_current_process(&backup);
}

TEST_F(DmellRedirectApplyTest, FailsCleanlyWithoutAConnectedBackend)
{
    dmell_redirect_t redirect = {0};
    redirect.stream = DMELL_STREAM_STDOUT;
    redirect.path = Dmod_StrDup("/some/file");

    dmell_redirect_backup_t backup;
    int result = dmell_redirect_apply_to_current_process(&redirect, 1, &backup);

    EXPECT_LT(result, 0);
    EXPECT_EQ(backup.touched_count, 0u);

    Dmod_Free(redirect.path);
}
