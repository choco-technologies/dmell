#ifndef DMELL_HLP_H
#define DMELL_HLP_H

#include <stddef.h>

/**
 * @brief Helper function to skip whitespaces in a command string.
 * 
 * @param str Current position in the command string
 * @param end_ptr Pointer to the end of the command string
 * @return const char* Pointer to the first non-whitespace character
 */
static inline const char* dmell_skip_whitespaces( const char* str, const char* end_ptr )
{
    const char* ptr = str;
    while(ptr < end_ptr && (*ptr == ' ' || *ptr == '\t') )
    {
        ptr++;
    }
    return ptr;
}

/**
 * @brief Helper function to add a string to a destination buffer.
 * 
 * @param dst Destination buffer
 * @param end_dst Pointer to the end of the destination buffer
 * @param value String to add
 * @param val_end Pointer to the end of the string to add
 * @return size_t Number of characters added
 */
static inline size_t dmell_add_to_string( char* dst, char* end_dst, const char* value, const char* val_end )
{
    if(value == NULL)
    {
        return 0;
    }
    size_t len = 0;
    while((*value) != '\0' && value < val_end)
    {
        if(dst != NULL && end_dst != NULL)
        {
            if(dst < end_dst)
            {
                *dst = *value;
            }
            dst++;
        }
        len++;
    }
    return len;
}

#endif // DMELL_HLP_H