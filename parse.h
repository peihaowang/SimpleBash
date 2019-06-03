#ifndef _PARSE_H_
#define _PARSE_H_

#include <string.h>

#define BUF_SIZE    1024
#define MAX_CMDS    100
#define MAX_ARGC    100
#define WHITE_CHARS " \f\n\r\t\v"
#define SEP_CHARS   " \f\n\r\t\v,()"

#define CAT_CONST_STR(STR1, STR2)   (STR1 STR2)

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

bool strstartswith(const char* str, const char* tar);
bool strendswith(const char* str, const char* tar);

/******************************************************************************
 * Command Utilities
 *****************************************************************************/

typedef struct
{
    int     argc;
    char*   argv[MAX_ARGC];

    char    input[BUF_SIZE];
    char    output[BUF_SIZE];
    bool    append;

} Command;

typedef struct
{
    int     cmdc;
    Command cmdv[MAX_CMDS];
    bool    bg;
} CommandLine;

void parse_command_line(CommandLine* command_line, char* line);

#endif /* _PARSE_H_ */
