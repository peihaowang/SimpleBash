#ifndef _PARSE_H_
#define _PARSE_H_

#include <string.h>

#define BUF_SIZE    1024
#define MAX_CMDS    100
#define MAX_ARGC    100
#define WHITE_CHARS " \f\n\r\t\v"
#define SEP_CHARS   " \f\n\r\t\v,()"

typedef int         bool;
#define true        (1)
#define false       (0)

/* This macro are useful to suppress the unsued variable warnings */
#define UNUSED(VAR) (void)(VAR)

/******************************************************************************
 * String Utilities
 *****************************************************************************/

char* strltrim(char* src, char* chars);
char* strrtrim(char* src, char* chars);
char* strtrim(char* src, char* chars);

/******************************************************************************
 * Command Utilities
 *****************************************************************************/

typedef struct
{
    int     argc;
    char*   argv[MAX_ARGC];

    char*   input;
    char*   output;

} Command;

typedef struct
{
    Command commands[MAX_CMDS];
    bool    background;
} CommandLine;

#endif /* _PARSE_H_ */
