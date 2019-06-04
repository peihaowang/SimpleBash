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
 * Job and job list
 *****************************************************************************/
typedef struct {
    pid_t pid;
    CommandLine* cmd_ln;
    char* wc;
    bool available;
} Job;

typedef struct {
    Job* data;
    int top;
} JobList;

void init_job_list(JobList* list)
{
    int i;
    list->data = (Job*)malloc(sizeof(Job) * BUF_SIZE);
    list->top = -1;

    for (i = 0; i < BUF_SIZE; i++) {
        (list->data[i]).available = false;
        (list->data[i]).wc = NULL;
        (list->data[i]).cmd_ln = NULL;
        (list->data[i]).pid = -1;
    }
}

void append_job_list(JobList* list, pid_t pid, CommandLine* cmd_ln, char* wc)
{
    ++list->top;

    (list->data[list->top]).pid = pid;
    (list->data[list->top]).cmd_ln = cmd_ln;
    (list->data[list->top]).wc = (char*)malloc(strlen(wc) + 1);
    (list->data[list->top]).available = true;

    strcpy((list->data[list->top]).wc, wc);
}

void remove_job_list(JobList* list, int idx)
{
    if (list == NULL || idx > list->top || !(list->data[idx]).available) {
        return;
    }

    (list->data[idx]).available = false;
    (list->data[idx]).pid = -1;

    if ((list->data[idx]).wc != NULL) {
        free((list->data[idx]).wc);
        (list->data[idx]).wc = NULL;
    }

    if ((list->data[idx]).wc != NULL) {
        free((list->data[idx]).wc);
        (list->data[idx]).wc = NULL;
    }

    (list->data[idx]).cmd_ln = NULL;

    if (idx != list->top) {
        return;
    }

    while (list->top > 0 && (list->data[--list->top]).available == false) { }
}

void print_job_list(JobList* list)
{
    int i;

    for (i = 0; i <= list->top; i++) {
        Job p = list->data[i];

        if (!p.available) {
            continue;
        }

        printf("[%d]%s  %s%*s%s\n", i + 1, "+", "Running", 21, "", "Test");

    }

}

JobList job_list;  /* the job list */


/******************************************************************************
 * Utilities
 *****************************************************************************/
/* get username */
void get_username(char* dest)
{
    char* user;
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
        exit(EXIT_FAILURE);
    }

    strcpy(dest, user);
}

/* get $HOME, e.g. /home/wyh */
void get_home_path(char* dest)
{
    char user[BUF_SIZE];
    char home_path[BUF_SIZE] = "/home/";

    /* get username */
    get_username(user);

    /* update home path */
    strcat(home_path, user);
    
    strcpy(dest, home_path);
}

/* replace $HOME with ~ */
void alias_home_path(char* dest, char* src)
{
    char home_path[BUF_SIZE];
    char alias_path[BUF_SIZE] = "~/";

    get_home_path(home_path);

    /* replace $HOME with ~ */
    if (strstartswith(src, home_path)) {
        path_change_directory(src, home_path, alias_path);
    }

    strcpy(dest, alias_path);
}

/* get current working directory where $HOME is replaced by ~ */
void get_cwd_with_alias_home(char* dest)
{
    char cwd[BUF_SIZE];
    getcwd(cwd, sizeof(cwd));
    alias_home_path(cwd, cwd);

    strcpy(dest, cwd);
}

/******************************************************************************
 * Parse and execute commands
 *****************************************************************************/
bool exec_builtin(Command* cmd)
{
    bool builtin = true;
    char* command_name;

    if (cmd->argc <= 0) {
        return false;
    }
    
    command_name = cmd->argv[0];
    if (strcmp(command_name, "cd") == 0) {
        char* dir;
        if (cmd->argc == 1) {
            char home_dir[BUF_SIZE];
            get_home_path(home_dir);
            dir = home_dir;
        } else {
            dir = cmd->argv[1];
        }
        if (chdir(dir) < 0) {
            fprintf(stderr, "%s: cd: %s: No such file or directory\n", PROGRAM_NAME, dir);
        }
    } else if (strcmp(command_name, "jobs") == 0) {
        puts("jobs");
    } else if (strcmp(command_name, "kill") == 0) {
        puts("kill");
    } else if (strcmp(command_name, "pwd") == 0) {
        char cwd[BUF_SIZE];
        getcwd(cwd, sizeof(cwd));

        printf("%s\n", cwd);
    } else if (strcmp(command_name, "exit") == 0) {
        exit(EXIT_SUCCESS);
    } else {
        builtin = false;
    }

    return builtin;
}

void exec_command(Command* cmd)
{
    if(cmd->argc <= 0) return;

    if(exec_builtin(cmd)){
        /* Exit child process */
        exit(EXIT_SUCCESS);
    }else{
        char command_name[BUF_SIZE]; strcpy(command_name, cmd->argv[0]);
        if(strchr(command_name, '/') == NULL){
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

        if(execv(command_name, cmd->argv) == -1){  /* command not found */
            fprintf(stderr, "%s: command not found\n", cmd->argv[0]);
            exit(EXIT_FAILURE);
        }
    }
}

void do_child_process(CommandLine* command_line, int idx, int pfd_input, int pfd_output)
{
    Command* cmd;
    int pfds[] = {-1, -1};
    bool is_child_proc = false;

    /* First leave -1 to initialize with the end command */
    if(idx < 0) idx = command_line->cmdc - 1;
    
    cmd = &command_line->cmdv[idx--];
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
            close(pfds[1]);
            dup2(pfds[0], STDIN_FILENO);
            close(pfds[0]);
        }

        if(output_file){
            /* Ouput redirection */
            int open_flags = O_RDWR | O_TRUNC | O_CREAT;
            int create_mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH;
            int output_fd = open(output_file, open_flags, create_mode);
            dup2(output_fd, STDOUT_FILENO);
            close(output_fd);
        }else if(pfd_input >= 0 && pfd_output >= 0){
            /* Pipe stdout to parent process */
            close(pfd_input);
            dup2(pfd_output, STDOUT_FILENO);
            close(pfd_output);
        }
        exec_command(cmd);
    }
}

void handle_line(char* line)
{
    bool child_process = false;
    CommandLine* command_line = (CommandLine*)malloc(sizeof(CommandLine));

    parse_command_line(command_line, line);

    if (command_line->cmdc > 0) {
        bool single_builtin = (command_line->cmdc == 1) && exec_builtin(&(command_line->cmdv[0]));

        if (!single_builtin) {
            pid_t pid = fork();
            child_process = (pid == 0);
            
            if (child_process) {  /* run in child process */
                do_child_process(command_line, -1, -1, -1);
            } else {
                /* push a job to job list */
                char cwd[BUF_SIZE];
                get_cwd_with_alias_home(cwd);
                append_job_list(&job_list, pid, command_line, cwd);
                
                if (!command_line->bg) {
                    int status = 0;
                    while (waitpid(pid, &status, 0) < 0) {
                        /* waiting */
                    }
                }
            }
        } else {  /* in single builtin case */
            free(command_line);
            free_command_line(command_line);
        }
    }
}


/******************************************************************************
 * Print prompt
 *****************************************************************************/
void print_prompt()
{
    char user[BUF_SIZE];
    char hostname[BUF_SIZE];
    char cwd[BUF_SIZE];

    /* get username */
    get_username(user);
    /* get hostname */
    gethostname(hostname, sizeof(hostname));
    /* get current working directory */
    get_cwd_with_alias_home(cwd);

    printf("%s@%s:%s$ ", user, hostname, cwd);
}


/******************************************************************************
 * Entrance: main
 *****************************************************************************/
int main(int argc, char* argv[])
{
    FILE* file;
    ShellMode sh_mode;
    char input_line[BUF_SIZE];

    /* init job list */
    init_job_list(&job_list);

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