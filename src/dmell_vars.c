#include <string.h>
#include <errno.h>
#include <stdbool.h>
#include "dmell_vars.h"
#include "dmell_hlp.h"
#include "dmod.h"

/**
 * @brief Helper function to check if a character is valid in a variable name.
 * 
 * @param c Character to check
 * @return true If the character is valid in a variable name
 * @return false Otherwise
 */
static bool is_var_name_char( char c )
{
    return ( ( c >= 'A' && c <= 'Z' ) ||
             ( c >= 'a' && c <= 'z' ) ||
             ( c >= '0' && c <= '9' ) ||
             ( c == '_' ) );
}

/**
 * @brief Helper function to check if the string at the current position represents a variable.
 * 
 * @param str Current position in the string
 * @param end_ptr Pointer to the end of the string
 * @return true If it is a variable
 * @return false Otherwise
 */
static bool is_var( const char* str, const char* end_ptr )
{
    if( str >= end_ptr || *str != '$' )
    {
        return false;
    }

    str++;
    if( str >= end_ptr || *str == '$' )
    {
        return false;
    }

    char c = *str;
    return ( c == '{' || is_var_name_char( c ) );
}

/**
 * @brief Helper function to get the end pointer of a variable in the string.
 * 
 * @param str Current position in the string
 * @param end_ptr Pointer to the end of the string
 * @return const char* Pointer to the position after the variable
 */
static const char* get_var_end( const char* str, const char* end_ptr )
{
    const char* ptr = str;
    if( ptr >= end_ptr || *ptr != '$' )
    {
        return NULL;
    }

    ptr++;
    if( ptr >= end_ptr )
    {
        return ptr;
    }

    if( *ptr == '{' )
    {
        ptr++;
        while( ptr < end_ptr && *ptr != '}' )
        {
            ptr++;
        }
        if( ptr < end_ptr && *ptr == '}' )
        {
            ptr++;
        }
        return ptr;
    }
    else
    {
        while( ptr < end_ptr && is_var_name_char( *ptr ) )
        {
            ptr++;
        }
        return ptr;
    }
}

/**
 * @brief Helper function to get the variable name from the string.
 * 
 * @param str Current position in the string
 * @param end_ptr Pointer to the end of the string
 * @param out_name_len Output parameter to hold the length of the variable name
 * @return const char* Pointer to the start of the variable name
 */
static const char* get_var_name( const char* str, const char* end_ptr, size_t *out_name_len )
{
    if(!is_var(str, end_ptr))
    {
        return NULL;
    }

    const char* var_end = get_var_end( str, end_ptr );
    if( var_end == NULL )
    {
        return NULL;
    }

    const char* name_start = str + 1;
    if( *name_start == '{' )
    {
        name_start++;
    }
    size_t name_len = var_end - name_start;
    if( *(var_end - 1) == '}' )
    {
        name_len--;
    }

    if( out_name_len != NULL )
    {
        *out_name_len = name_len;
    }
    return name_start;
}

/**
 * @brief Helper function to find the next variable in the string.
 * 
 * @param str Current position in the string
 * @param end_ptr Pointer to the end of the string
 * @return const char* Pointer to the position of the next variable, or end_ptr if none found
 */
static const char* find_next_var( const char* str, const char* end_ptr )
{
    const char* ptr = str;
    while( ptr < end_ptr )
    {
        if( is_var( ptr, end_ptr ) )
        {
            return ptr;
        }
        ptr++;
    }
    return end_ptr;
}

/**
 * @brief Adds a new variable to the list.
 * 
 * @param head Pointer to the head of the variable list
 * @param name Name of the variable to add
 * @param value Value of the variable to add
 * @return dmell_var_t* Pointer to the head of the updated variable list
 */
dmell_var_t* dmell_add_variable( dmell_var_t* head, const char* name, const char* value)
{
    if(name == NULL || value == NULL)
    {
        DMOD_LOG_ERROR("Invalid arguments to dmell_add_variable: %p, %p\n", name, value);
        return head;
    }

    dmell_var_t* new_var = (dmell_var_t*)Dmod_Malloc(sizeof(dmell_var_t));
    if(new_var == NULL)
    {
        DMOD_LOG_ERROR("Memory allocation failed in dmell_add_variable for %s=%s\n", name, value);
        return head;
    }

    new_var->name = Dmod_StrDup(name);
    new_var->value = Dmod_StrDup(value);
    if(new_var->name == NULL || new_var->value == NULL)
    {
        DMOD_LOG_ERROR("Memory allocation failed in Dmod_StrDup in dmell_add_variable for %s=%s\n", name, value);
        Dmod_Free(new_var->name);
        Dmod_Free(new_var->value);
        Dmod_Free(new_var);
        return head;    
    }

    new_var->next = NULL;

    // If the list is empty, the new variable becomes the head
    if(head == NULL)
    {
        return new_var;
    }

    // Find the last element in the list
    dmell_var_t* current = head;
    while(current->next != NULL)
    {
        current = current->next;
    }

    // Add the new variable at the end
    current->next = new_var;
    return head;
}

/**
 * @brief Finds a variable by its name.
 * 
 * @param head Pointer to the head of the variable list
 * @param name Name of the variable to find
 * @return dmell_var_t* Pointer to the found variable, or NULL if not found
 */
dmell_var_t* dmell_find_variable( dmell_var_t* head, const char* name )
{
    dmell_var_t* current = head;
    while(current != NULL)
    {
        if(strcmp(current->name, name) == 0)
        {
            return current;
        }
        current = current->next;
    }
    return NULL;
}

/**
 * @brief Removes a variable from the list by its name.
 * 
 * @param head Pointer to the head of the variable list
 * @param name Name of the variable to remove
 * @return dmell_var_t* Pointer to the head of the updated variable list
 */
dmell_var_t* dmell_remove_variable( dmell_var_t* head, const char* name )
{
    dmell_var_t* current = head;
    dmell_var_t* previous = NULL;

    while(current != NULL)
    {
        if(strcmp(current->name, name) == 0)
        {
            if(previous == NULL)
            {
                head = current->next;
            }
            else
            {
                previous->next = current->next;
            }
            Dmod_Free(current->name);
            Dmod_Free(current->value);
            Dmod_Free(current);
            return head;
        }
        previous = current;
        current = current->next;
    }
    return head;
}

/**
 * @brief Adds variables for each argument in the format ARG0, ARG1, ..., ARGN-1.
 * 
 * @param head Pointer to the head of the variable list
 * @param argc Number of arguments
 * @param argv Array of argument strings
 * @return dmell_var_t* Pointer to the head of the updated variable list
 */
dmell_var_t* dmell_add_argv_variables( dmell_var_t* head, int argc, char** argv )
{
    for(int i = 0; i < argc; i++)
    {
        char var_name[32];
        Dmod_SnPrintf(var_name, sizeof(var_name), "%d", i);
        head = dmell_add_variable( head, var_name, argv[i] );
    }
    return head;
}

/**
 * @brief Frees the entire variable list.
 * 
 * @param head Pointer to the head of the variable list
 */
void dmell_free_variables( dmell_var_t* head )
{
    dmell_var_t* current = head;
    while(current != NULL)
    {
        dmell_var_t* next = current->next;
        Dmod_Free(current->name);
        Dmod_Free(current->value);
        Dmod_Free(current);
        current = next;
    }
}

/**
 * @brief Sets the value of a variable. If the variable does not exist, it is added.
 * 
 * @param head Pointer to the head of the variable list
 * @param name Name of the variable to set
 * @param value Value to set for the variable
 * @return dmell_var_t* Pointer to the head of the updated variable list
 */
dmell_var_t* dmell_set_variable( dmell_var_t* head, const char* name, const char* value )
{
    if(name == NULL || value == NULL)
    {
        DMOD_LOG_ERROR("Invalid arguments to dmell_set_variable: %p, %p\n", name, value);
        return head;
    }
    int result = Dmod_SetEnv( name, value, 1 );
    if( result != 0 )
    {
        DMOD_LOG_ERROR("Failed to set environment variable in dmell_set_variable: %s=%s\n", name, value);
    }

    dmell_var_t* var = dmell_find_variable( head, name );
    if( var != NULL )
    {
        Dmod_Free( var->value );
        var->value = Dmod_StrDup( value );
        if( var->value == NULL )
        {
            DMOD_LOG_ERROR("Memory allocation failed in Dmod_StrDup in dmell_set_variable for %s=%s\n", name, value);
        }
        return head;
    }
    else
    {
        return dmell_add_variable( head, name, value );
    }
}

/**
 * @brief Gets the value of a variable by its name.
 * 
 * @param head Pointer to the head of the variable list
 * @param name Name of the variable to get
 * @return const char* Value of the variable, or NULL if not found
 */
const char* dmell_get_variable_value( dmell_var_t* head, const char* name )
{
    dmell_var_t* var = dmell_find_variable( head, name );
    if( var != NULL )
    {
        return var->value;
    }
    return Dmod_GetEnv( name );
}

/**
 * @brief Expands variables in a string and writes the result to the destination buffer.
 * 
 * @note If dst is NULL, the function only calculates the required buffer size.
 * 
 * @param head Pointer to the head of the variable list
 * @param str Input string with variables to expand
 * @param str_len Length of the input string
 * @param dst [optional] Destination buffer to write the expanded string
 * @param dst_size [optional] Size of the destination buffer
 * 
 * @return number of characters written to dst (excluding null terminator) or -errno on error
 */
int dmell_expand_variables( dmell_var_t* head, const char* str, size_t str_len, char* dst, size_t dst_size )
{
    if(str == NULL)
    {
        DMOD_LOG_ERROR("Invalid argument to dmell_expand_variables: %p\n", str);
        return -EINVAL;
    }

    size_t required_size = 0;

    char* dst_ptr = dst;
    char* end_dst = dst != NULL ? dst + dst_size : NULL;
    const char* end_ptr = str + str_len;
    const char* ptr = str;
    while( ptr < end_ptr )
    {
        ptr = dmell_skip_whitespaces( ptr, end_ptr );
        const char* var_start = find_next_var( ptr, end_ptr );
        required_size += dmell_add_to_string( dst_ptr, end_dst, ptr, var_start );
        dst_ptr = dst != NULL ? dst + required_size : NULL;
        if(var_start < end_ptr)
        {
            size_t name_len = 0;
            const char* var_name = get_var_name(var_start, end_ptr, &name_len);
            if(var_name != NULL && name_len > 0 && name_len < DMELL_MAX_VAR_NAME_LEN)
            {
                char var_name_cpy[ name_len + 1 ];
                strncpy( var_name_cpy, var_name, name_len );
                var_name_cpy[ name_len ] = '\0';
                const char* var_value = dmell_get_variable_value(head, var_name_cpy);
                const char* var_value_end = var_value != NULL ? var_value + strlen(var_value) : NULL;
                required_size += dmell_add_to_string(dst_ptr, end_dst, var_value, var_value_end);
                dst_ptr = dst != NULL ? dst + required_size : NULL;
            }
            ptr = get_var_end( var_start, end_ptr );
        }
        else 
        {
            ptr = end_ptr;
        }
    }

    return (int)required_size;
}