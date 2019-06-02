#include "parse.h"

/******************************************************************************
 * String Utilities
 *****************************************************************************/

char *strltrim(char *src, char *chars)
{
    char ch;
    while ((ch = *src) != '\0')
    {
        char *found = strchr(chars, ch);
        if (found == NULL)
            break;
        src++;
    }
    return src;
}

char *strrtrim(char *src, char *chars)
{
    char *rev = src + strlen(src) - 1;
    while (rev >= src)
    {
        char *found = strchr(chars, *rev);
        if (found == NULL)
            break;
        (*rev) = '\0';
        rev--;
    }
    return src;
}

char *strtrim(char *src, char *chars)
{
    return strrtrim(strltrim(src, chars), chars);
}
