#include "parse.h"

/******************************************************************************
 * String Utilities
 *****************************************************************************/

char* strltrim(char* src, const char* chars)
{
    char ch;
    while((ch = *src) != '\0'){
        char* found = strchr(chars, ch);
        if(found == NULL) break;
        src++;
    }
    return src;
}

char* strrtrim(char* src, const char* chars)
{
    char* rev = src + strlen(src) - 1;
    while(rev >= src){
        char* found = strchr(chars, *rev);
        if (found == NULL) break;
        (*rev) = '\0';
        rev--;
    }
    return src;
}

char* strtrim(char* src, const char* chars)
{
    return strrtrim(strltrim(src, chars), chars);
}

bool strstartswith(const char* str, const char* tar)
{
    if(strlen(str) < strlen(tar)) return false;
    return strncmp(str, tar, strlen(tar)) == 0;
}

bool strendswith(const char* str, const char* tar)
{
    int pos = strlen(str) - strlen(tar);
    if(pos < 0) return false;
    return strcmp(str + pos, tar) == 0;
}

/******************************************************************************
 * Path Utilities
 *****************************************************************************/

char* path_cat(char* dest, char* src)
{
    dest = strrtrim(dest, "/");
    src = strltrim(dest, "/");
    return strcat(dest, src);
}

char* path_change_directory(char* path, char* parent, char* target)
{
    parent = path_eliminate_tail_slash(parent);
    if(strstartswith(path, parent)){
        target = path_cat(target, path + strlen(parent));
    }
    return target;
}

char* path_eliminate_begin_slash(char* path)
{
    return strltrim(path, "/");
}

char* path_eliminate_tail_slash(char* path)
{
    return strrtrim(path, "/");
}

char* path_ensure_tail_slash(char* path)
{
    if(!strendswith(path, "/")){
        strcat(path, "/");
    }
    return path;
}

/******************************************************************************
 * Command Utilities
 *****************************************************************************/

void parse_command_line(CommandLine* command_line, char* line)
{
    const char* pipe_delimiters = "|";
    char* token;
    char* commands[MAX_CMDS];
    int i;

    line = strtrim(line, WHITE_CHARS);
    /* Is background command? */
    if(strendswith(line, "&")){
        command_line->bg = true;
    }
    /* Eliminate the end &, after it's handled */
    line = strtrim(line, CAT_CONST_STR(WHITE_CHARS, "&"));

    /* Tokenize by vertical line(|) */
    i = 0;
    token = strtok(line, pipe_delimiters);
    while(token != NULL){
        commands[i++] = token;
        token = strtok(NULL, pipe_delimiters);
    }
    /* Now i equals to the count of arguments */
    command_line->cmdc = i;
    for(i = 0; i < command_line->cmdc; i++){
        int argc = 0;
        Command* cmd = &command_line->cmdv[i];

        /* Traverse each token */
        token = strtok(commands[i], WHITE_CHARS);
        while(token != NULL){
            if(strcmp(token, ">") == 0){
                /* Write */
                token = strtok(NULL, WHITE_CHARS);
                strcpy(cmd->output, token);
                cmd->append = false;
            }else if(strcmp(token, ">>") == 0){
                /* Append */
                token = strtok(NULL, WHITE_CHARS);
                strcpy(cmd->output, token);
                cmd->append = true;
            }else if(strcmp(token, "<") == 0){
                /* Read */
                token = strtok(NULL, WHITE_CHARS);
                strcpy(cmd->input, token);
                cmd->append = false;
            }else{
                strcpy(cmd->argv[argc++], token);
            }
            token = strtok(NULL, WHITE_CHARS);
        }
        cmd->argc = argc;
    }
}
