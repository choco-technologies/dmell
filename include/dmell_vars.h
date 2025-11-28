#ifndef DMELL_VARS_H
#define DMELL_VARS_H

#include <stddef.h>

#ifndef DMELL_MAX_VAR_NAME_LEN
/**
 * @brief Maximum length for variable names.
 */
#   define DMELL_MAX_VAR_NAME_LEN 256
#endif

typedef struct dmell_var_s
{
    char* name;                 /**< Name of the variable */
    char* value;                /**< Value of the variable */
    struct dmell_var_s* next;   /**< Pointer to the next variable in the list */
} dmell_var_t;

extern dmell_var_t* dmell_add_variable( dmell_var_t* head, const char* name, const char* value);
extern dmell_var_t* dmell_find_variable( dmell_var_t* head, const char* name );
extern dmell_var_t* dmell_remove_variable( dmell_var_t* head, const char* name );
extern dmell_var_t* dmell_add_argv_variables( dmell_var_t* head, int argc, char** argv );
extern void dmell_free_variables( dmell_var_t* head );
extern dmell_var_t* dmell_set_variable( dmell_var_t* head, const char* name, const char* value );
extern const char* dmell_get_variable_value( dmell_var_t* head, const char* name );
extern int dmell_expand_variables( dmell_var_t* head, const char* str, size_t str_len, char* dst, size_t dst_size );

#endif // DMELL_VARS_H