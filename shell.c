#ifndef _DEFAULT_SOURCE
#define _DEFAULT_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <pwd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/unistd.h>

#include "parse.h"

#define PROGRAM_NAME "shell"


/**
 * enum of shell mode: interactive/noninteractive
 */
typedef enum {
    interactive,
    noninteractive
} ShellMode;

/*void handle_line(char* line)
{
    bool child_process = false;
    CommandLine command_line = new_command_line(line);

    if (!command_line.is_empty()) {
        bool single_buildin = command_line.single_command() && exec_buildin(&command_line.m_commands[0]);

        if (!single_buildin) {
            
        }
    }
}*/
void print_prompt()
{
    char* user;
    char hostname[1024];
    char cwd[1024];

    /* get username */
    {
        uid_t uid;
        struct passwd* pwd;
        
        /* get uid */
        uid = geteuid();
        /* get user profile */
        pwd = getpwuid(uid);
        if (pwd) {
            user = pwd->pw_name;
        } else {  /* failed to get username */
            fprintf(stderr, "%s: cannot find username for UID %u\n", PROGRAM_NAME, (unsigned)uid);
            exit (EXIT_FAILURE);
        }
    }
    /* get hostname */
    gethostname(hostname, sizeof(hostname));
    /* get current working directory */
    getcwd(cwd, sizeof(cwd));

    /* TODO */
    /* replace $HOME with ~ */

    printf("%s@%s:%s$ ", user, hostname, cwd);
}

int main(int argc, char* argv[])
{
    FILE* file;
    ShellMode sh_mode;
    char input_line[1024];

    /* clear input line */
    memset(input_line, 0, sizeof(input_line));

    if (argc <= 1) {  /* interactive mode */
        /* set mode to interactive */
        sh_mode = interactive;
        /* receive input from stdin */
        file = stdin;
    } else {    /* non-interactive mode */
        char* filename;

        /* set mode to non-interactive */
        sh_mode = noninteractive;
        
        /* open input file */
        filename = argv[1];
        file = fopen(filename, "r");

        /* failed to open file */
        if (file == NULL) {
            /* not found message */
            fprintf(stderr, "%s: %s: No such file or directory\n", PROGRAM_NAME, filename);
            /* close file */
            fclose(file);

            exit(EXIT_FAILURE);
        }
    }

    do {
        /* print prompt in interactive mode */
        if (sh_mode == interactive) {
            print_prompt();
        }

        /* handle input line */
        printf("%s", input_line);
    } while (fgets(input_line, sizeof(input_line), file) != NULL);

    /* close input file in noninteractive mode */
    if (sh_mode == noninteractive) {
        fclose(file);
    }

    return 0;
}

#endif