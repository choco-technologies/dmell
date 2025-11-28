#include <string.h>
#include "dmell_vars.h"
#include "dmod.h"

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

    new_var->next = head;
    return new_var;
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
    return NULL;
}
