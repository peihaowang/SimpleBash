#ifndef _DEFAULT_SOURCE
#define _DEFAULT_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <pwd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/unistd.h>
#include <fcntl.h>

#include "parse.h"

#define PROGRAM_NAME "shell"


/******************************************************************************
 * Enum of shell mode: interactive/noninteractive
 *****************************************************************************/
typedef enum {
    interactive,
    noninteractive
} ShellMode;


/******************************************************************************
 * Parse and execute commands
 *****************************************************************************/
bool exec_buildin(Command* cmd)
{
    bool buildin = true;
    char* command_name;

    if (cmd->argc <= 0) {
        return false;
    }
    
    command_name = cmd->argv[0];
    if (strcmp(command_name, "cd") == 0) {
        printf("cd");
    } else if (strcmp(command_name, "jobs") == 0) {
        printf("jobs");
    } else if (strcmp(command_name, "kill") == 0) {
        printf("kill");
    } else if (strcmp(command_name, "pwd") == 0) {
        printf("pwd");
    } else if (strcmp(command_name, "exit") == 0) {
        printf("exit");
    } else{
        buildin = false;
    }

    return buildin;
}

void exec_command(Command* cmd)
{
    if(cmd->argc <= 0) return;

    if(exec_buildin(cmd)){
        /* Exit child process */
        exit(0);
    }else{
        char command_name[BUF_SIZE]; strcpy(command_name, cmd->argv[0]);
        if(strchr(command_name, '/')){
            char* env_str = getenv("PATH");
            char* env_dir = strtok(env_str, ":");
            while(env_dir != NULL){
                char abs_path[BUF_SIZE]; strcpy(abs_path, env_dir);
                path_cat(abs_path, command_name);
                if(path_file_exists(abs_path)){
                    strcpy(command_name, abs_path);
                    break;
                }
                env_dir = strtok(NULL, ":");
            }
        }
        execv(command_name, cmd->argv);
    }
}

void do_child_process(CommandLine* command_line, int idx, int pfd_input, int pfd_output)
{
    /* First leave -1 to initialize with the end command */
    if(idx < 0) idx = command_line->cmdc - 1;
    
    if(idx >= 0){
        Command* cmd = &command_line->cmdv[idx--];
        int pfds[] = {-1, -1};
        bool is_child_proc = false;
        if(idx >= 0){
            if(pipe(pfds) == 0){
                is_child_proc = (fork() == 0);
                if(is_child_proc){
                    do_child_process(command_line, idx, pfds[0], pfds[1]);
                }
            }
        }
        if(!is_child_proc){
            char* input_file = cmd->input;
            char* output_file = cmd->output;

            if(input_file){
                /*  Input redirection */
                int input_fd = open(input_file, O_RDONLY);
                dup2(input_fd, STDIN_FILENO);
                close(input_fd);
            }else if(pfds[0] >= 0 && pfds[1] >= 0){
                /* Pipe stdin from child process */
                close(STDIN_FILENO);
                dup2(pfds[0], STDIN_FILENO);
                close(pfds[1]);
            }

            if(output_file){
                /* Ouput redirection */
                int open_flags = O_RDWR | O_TRUNC | O_CREAT;
                int create_mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH;
                int output_fd = open(output_file, open_flags, create_mode);
                dup2(output_fd, STDOUT_FILENO);
                close(output_fd);
            }else if(pfds[0] >= 0 && pfds[1] >= 0){
                /* Pipe stdout to parent process */
                close(STDOUT_FILENO);
                dup2(pfd_output, STDOUT_FILENO);
                close(pfd_input);
            }
            exec_command(cmd);
        }
    }
}

void handle_line(char* line)
{
    bool child_process = false;
    CommandLine command_line;

    UNUSED(child_process);
    
    parse_command_line(&command_line, line);

    if (command_line.cmdc > 0) {
        bool single_buildin = (command_line.cmdc == 1) && exec_buildin(&command_line.cmdv[0]);

        if (!single_buildin) {
            
        }
    }

    free_command_line(&command_line);
}


/******************************************************************************
 * Print prompt
 *****************************************************************************/
void print_prompt()
{
    char* user;
    char hostname[BUF_SIZE];
    char cwd[BUF_SIZE];
    char home_path[BUF_SIZE] = "/home/";
    char path_alias[BUF_SIZE] = "~/";

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

    /* update home path */
    strcat(home_path, user);
    /* replace $HOME with ~ */
    path_change_directory(cwd, home_path, path_alias);

    printf("%s@%s:%s$ ", user, hostname, path_alias);
}


/******************************************************************************
 * Entrance: main
 *****************************************************************************/
int main(int argc, char* argv[])
{
    FILE* file;
    ShellMode sh_mode;
    char input_line[BUF_SIZE];

    /* clear input line */
    memset(input_line, 0, sizeof(input_line));

    if (argc <= 1) {  /* interactive mode */
        /* set mode to interactive */
        sh_mode = interactive;
        /* receive input from stdin */
        file = stdin;
    } else {  /* non-interactive mode */
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
        /* handle input line */
        handle_line(input_line);

        /* print prompt in interactive mode */
        if (sh_mode == interactive) {
            print_prompt();
        }
    } while (fgets(input_line, sizeof(input_line), file) != NULL);

    /* close input file in noninteractive mode */
    if (sh_mode == noninteractive) {
        fclose(file);
    }

    return 0;
}

#endif