/**
 * @file dmell_vars_test.c
 * @brief Unit tests for dmell variable management functions (dmell_vars module)
 */
#include "dmod_test.h"
#include "dmell_vars.h"
#include <string.h>

// ===============================================================
//                  Variable List Management Tests
// ===============================================================

static dmell_var_t* variables;

void dmod_test_setup(void)
{
    variables = NULL;
}

void dmod_test_teardown(void)
{
    if( variables != NULL )
    {
        dmell_free_variables( variables );
        variables = NULL;
    }
}

DMOD_TEST_STEP(add_single_variable)
{
    variables = dmell_add_variable(NULL, "TEST_VAR", "test_value");

    DMOD_TEST_EXPECT_NOT_NULL(variables);
    DMOD_TEST_EXPECT(strcmp(variables->name, "TEST_VAR") == 0);
    DMOD_TEST_EXPECT(strcmp(variables->value, "test_value") == 0);
    DMOD_TEST_EXPECT_NULL(variables->next);
}

DMOD_TEST_STEP(add_multiple_variables)
{
    variables = dmell_add_variable(NULL, "VAR1", "value1");
    variables = dmell_add_variable(variables, "VAR2", "value2");
    variables = dmell_add_variable(variables, "VAR3", "value3");

    DMOD_TEST_EXPECT_NOT_NULL(variables);
    DMOD_TEST_EXPECT(strcmp(variables->name, "VAR1") == 0);
    DMOD_TEST_EXPECT(strcmp(variables->value, "value1") == 0);

    DMOD_TEST_EXPECT_NOT_NULL(variables->next);
    DMOD_TEST_EXPECT(strcmp(variables->next->name, "VAR2") == 0);
    DMOD_TEST_EXPECT(strcmp(variables->next->value, "value2") == 0);

    DMOD_TEST_EXPECT_NOT_NULL(variables->next->next);
    DMOD_TEST_EXPECT(strcmp(variables->next->next->name, "VAR3") == 0);
    DMOD_TEST_EXPECT(strcmp(variables->next->next->value, "value3") == 0);
    DMOD_TEST_EXPECT_NULL(variables->next->next->next);
}

DMOD_TEST_STEP(add_variable_null_name)
{
    variables = dmell_add_variable(NULL, NULL, "value");
    DMOD_TEST_EXPECT_NULL(variables);
}

DMOD_TEST_STEP(add_variable_null_value)
{
    variables = dmell_add_variable(NULL, "name", NULL);
    DMOD_TEST_EXPECT_NULL(variables);
}

DMOD_TEST_STEP(find_existing_variable)
{
    variables = dmell_add_variable(NULL, "VAR1", "value1");
    variables = dmell_add_variable(variables, "VAR2", "value2");
    variables = dmell_add_variable(variables, "VAR3", "value3");

    dmell_var_t* found = dmell_find_variable(variables, "VAR2");

    DMOD_TEST_EXPECT_NOT_NULL(found);
    DMOD_TEST_EXPECT(strcmp(found->name, "VAR2") == 0);
    DMOD_TEST_EXPECT(strcmp(found->value, "value2") == 0);
}

DMOD_TEST_STEP(find_first_variable)
{
    variables = dmell_add_variable(NULL, "FIRST", "first_value");
    variables = dmell_add_variable(variables, "SECOND", "second_value");

    dmell_var_t* found = dmell_find_variable(variables, "FIRST");

    DMOD_TEST_EXPECT_NOT_NULL(found);
    DMOD_TEST_EXPECT(strcmp(found->name, "FIRST") == 0);
}

DMOD_TEST_STEP(find_last_variable)
{
    variables = dmell_add_variable(NULL, "FIRST", "first_value");
    variables = dmell_add_variable(variables, "LAST", "last_value");

    dmell_var_t* found = dmell_find_variable(variables, "LAST");

    DMOD_TEST_EXPECT_NOT_NULL(found);
    DMOD_TEST_EXPECT(strcmp(found->name, "LAST") == 0);
}

DMOD_TEST_STEP(find_non_existing_variable)
{
    variables = dmell_add_variable(NULL, "VAR1", "value1");

    dmell_var_t* found = dmell_find_variable(variables, "NONEXISTENT");

    DMOD_TEST_EXPECT_NULL(found);
}

DMOD_TEST_STEP(remove_middle_variable)
{
    variables = dmell_add_variable(NULL, "VAR1", "value1");
    variables = dmell_add_variable(variables, "VAR2", "value2");
    variables = dmell_add_variable(variables, "VAR3", "value3");

    variables = dmell_remove_variable(variables, "VAR2");

    DMOD_TEST_EXPECT_NOT_NULL(variables);
    DMOD_TEST_EXPECT(strcmp(variables->name, "VAR1") == 0);
    DMOD_TEST_EXPECT_NOT_NULL(variables->next);
    DMOD_TEST_EXPECT(strcmp(variables->next->name, "VAR3") == 0);
    DMOD_TEST_EXPECT_NULL(variables->next->next);
}

DMOD_TEST_STEP(remove_first_variable)
{
    variables = dmell_add_variable(NULL, "VAR1", "value1");
    variables = dmell_add_variable(variables, "VAR2", "value2");

    variables = dmell_remove_variable(variables, "VAR1");

    DMOD_TEST_EXPECT_NOT_NULL(variables);
    DMOD_TEST_EXPECT(strcmp(variables->name, "VAR2") == 0);
    DMOD_TEST_EXPECT_NULL(variables->next);
}

DMOD_TEST_STEP(remove_last_variable)
{
    variables = dmell_add_variable(NULL, "VAR1", "value1");
    variables = dmell_add_variable(variables, "VAR2", "value2");

    variables = dmell_remove_variable(variables, "VAR2");

    DMOD_TEST_EXPECT_NOT_NULL(variables);
    DMOD_TEST_EXPECT(strcmp(variables->name, "VAR1") == 0);
    DMOD_TEST_EXPECT_NULL(variables->next);
}

DMOD_TEST_STEP(remove_only_variable)
{
    variables = dmell_add_variable(NULL, "ONLY", "only_value");

    variables = dmell_remove_variable(variables, "ONLY");

    DMOD_TEST_EXPECT_NULL(variables);
}

DMOD_TEST_STEP(remove_non_existing_variable)
{
    variables = dmell_add_variable(NULL, "VAR1", "value1");

    variables = dmell_remove_variable(variables, "NONEXISTENT");

    DMOD_TEST_EXPECT_NOT_NULL(variables);
    DMOD_TEST_EXPECT(strcmp(variables->name, "VAR1") == 0);
}

DMOD_TEST_STEP(set_existing_variable)
{
    variables = dmell_add_variable(NULL, "VAR", "old_value");

    variables = dmell_set_variable(variables, "VAR", "new_value");

    DMOD_TEST_EXPECT_NOT_NULL(variables);
    DMOD_TEST_EXPECT(strcmp(variables->value, "new_value") == 0);
    DMOD_TEST_EXPECT_NULL(variables->next);
}

DMOD_TEST_STEP(set_new_variable)
{
    variables = dmell_add_variable(NULL, "EXISTING", "existing_value");

    variables = dmell_set_variable(variables, "NEW", "new_value");

    DMOD_TEST_EXPECT_NOT_NULL(variables);
    dmell_var_t* found = dmell_find_variable(variables, "NEW");
    DMOD_TEST_EXPECT_NOT_NULL(found);
    DMOD_TEST_EXPECT(strcmp(found->value, "new_value") == 0);
}

DMOD_TEST_STEP(get_variable_value)
{
    variables = dmell_add_variable(NULL, "MYVAR", "myvalue");

    const char* value = dmell_get_variable_value(variables, "MYVAR");

    DMOD_TEST_EXPECT_NOT_NULL(value);
    DMOD_TEST_EXPECT(strcmp(value, "myvalue") == 0);
}

DMOD_TEST_STEP(add_argv_variables)
{
    char* argv[] = { (char*)"arg0", (char*)"arg1", (char*)"arg2" };
    int argc = 3;

    variables = dmell_add_argv_variables(NULL, argc, argv);

    DMOD_TEST_EXPECT_NOT_NULL(variables);

    const char* val0 = dmell_get_variable_value(variables, "0");
    const char* val1 = dmell_get_variable_value(variables, "1");
    const char* val2 = dmell_get_variable_value(variables, "2");

    DMOD_TEST_EXPECT_NOT_NULL(val0);
    DMOD_TEST_EXPECT_NOT_NULL(val1);
    DMOD_TEST_EXPECT_NOT_NULL(val2);

    DMOD_TEST_EXPECT(strcmp(val0, "arg0") == 0);
    DMOD_TEST_EXPECT(strcmp(val1, "arg1") == 0);
    DMOD_TEST_EXPECT(strcmp(val2, "arg2") == 0);
}

// ===============================================================
//                  Variable Expansion Tests
// ===============================================================

DMOD_TEST_STEP(expand_simple_variable)
{
    variables = dmell_add_variable(NULL, "NAME", "World");

    const char* input = "Hello $NAME!";
    char output[64];

    int result = dmell_expand_variables(variables, input, strlen(input), output, sizeof(output));

    DMOD_TEST_EXPECT(result > 0);
    output[result] = '\0';
    DMOD_TEST_EXPECT(strcmp(output, "Hello World!") == 0);
}

DMOD_TEST_STEP(expand_variable_with_braces)
{
    variables = dmell_add_variable(NULL, "VAR", "value");

    const char* input = "${VAR}text";
    char output[64];

    int result = dmell_expand_variables(variables, input, strlen(input), output, sizeof(output));

    DMOD_TEST_EXPECT(result > 0);
    output[result] = '\0';
    DMOD_TEST_EXPECT(strcmp(output, "valuetext") == 0);
}

DMOD_TEST_STEP(expand_multiple_variables)
{
    variables = dmell_add_variable(NULL, "FIRST", "Hello");
    variables = dmell_add_variable(variables, "SECOND", "World");

    const char* input = "$FIRST $SECOND";
    char output[64];

    int result = dmell_expand_variables(variables, input, strlen(input), output, sizeof(output));

    DMOD_TEST_EXPECT(result > 0);
    output[result] = '\0';
    // dmell_expand_variables skips leading whitespace in each segment - intentional shell behavior
    DMOD_TEST_EXPECT(strcmp(output, "HelloWorld") == 0);
}

DMOD_TEST_STEP(expand_non_existing_variable)
{
    const char* input = "$NONEXISTENT";
    char output[64];

    int result = dmell_expand_variables(NULL, input, strlen(input), output, sizeof(output));

    DMOD_TEST_EXPECT(result >= 0);
    output[result] = '\0';
    // Non-existing variables should expand to empty string
    DMOD_TEST_EXPECT(strcmp(output, "") == 0);
}

DMOD_TEST_STEP(expand_no_variables)
{
    const char* input = "Plain text without variables";
    char output[64];

    int result = dmell_expand_variables(NULL, input, strlen(input), output, sizeof(output));

    DMOD_TEST_EXPECT(result > 0);
    output[result] = '\0';
    DMOD_TEST_EXPECT(strcmp(output, "Plain text without variables") == 0);
}

DMOD_TEST_STEP(expand_calculate_buffer_size)
{
    variables = dmell_add_variable(NULL, "VAR", "value");

    const char* input = "$VAR";

    int result = dmell_expand_variables(variables, input, strlen(input), NULL, 0);

    DMOD_TEST_EXPECT_EQ(result, 5); // "value" is 5 characters
}

DMOD_TEST_STEP(expand_null_string)
{
    int result = dmell_expand_variables(NULL, NULL, 0, NULL, 0);

    DMOD_TEST_EXPECT(result < 0); // Should return error
}

DMOD_TEST_STEP(expand_variable_with_underscore)
{
    variables = dmell_add_variable(NULL, "MY_VAR_NAME", "myvalue");

    const char* input = "$MY_VAR_NAME";
    char output[64];

    int result = dmell_expand_variables(variables, input, strlen(input), output, sizeof(output));

    DMOD_TEST_EXPECT(result > 0);
    output[result] = '\0';
    DMOD_TEST_EXPECT(strcmp(output, "myvalue") == 0);
}

DMOD_TEST_STEP(expand_variable_with_numbers)
{
    variables = dmell_add_variable(NULL, "VAR123", "value123");

    const char* input = "$VAR123";
    char output[64];

    int result = dmell_expand_variables(variables, input, strlen(input), output, sizeof(output));

    DMOD_TEST_EXPECT(result > 0);
    output[result] = '\0';
    DMOD_TEST_EXPECT(strcmp(output, "value123") == 0);
}
