#include <ctype.h>
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <signal.h>
#include <sys/wait.h>
#include <termios.h>
#include <unistd.h>

#include "tokenizer.h"

/* Convenience macro to silence compiler warnings about unused function parameters. */
#define unused __attribute__((unused))

/* Whether the shell is connected to an actual terminal or not. */
bool shell_is_interactive;

/* File descriptor for the shell input */
int shell_terminal;

/* Terminal mode settings for the shell */
struct termios shell_tmodes;

/* Process group id for the shell */
pid_t shell_pgid;

int cmd_exit(struct tokens *tokens);
int cmd_help(struct tokens *tokens);
int cmd_pwd(struct tokens *tokens);
int cmd_cd(struct tokens *tokens);
int cmd_buildin(struct tokens *tokens);
int cmd_shell(struct tokens *tokens);

/* Built-in command functions take token array (see parse.h) and return int */
typedef int cmd_fun_t(struct tokens *tokens);

/* Built-in command struct and lookup table */
typedef struct fun_desc {
  cmd_fun_t *fun;
  char *cmd;
  char *doc;
} fun_desc_t;

fun_desc_t cmd_table[] = {
    {cmd_help, "?", "show this help menu"},
    {cmd_exit, "exit", "exit the command shell"},
    //my code
    {cmd_pwd, "pwd", "show current working directory"},
    {cmd_cd, "cd", "change directory"},
    {cmd_shell, "other", "use any shell"},
    {cmd_buildin, "buildin", "change directory"},
};

/* shell cmd*/
int cmd_shell(struct tokens *tokens){
    char *cmd = tokens_get_token(tokens,0);
    const char *s = getenv("PATH");
    char path[1024];
    bzero(path,1024);
    int leng =0;
    for(int i=0; i<strlen(s); i++){
        char ch = s[i];
        if(ch==':'|| (i+1) == strlen(s)){
            //i++;
            strcat(path, "/");
            strcat(path,cmd);
            //check whether the environenment path variable contains the file we want to execute
            if(access(path,F_OK)==0) {
                //printf("%s\n", path);
                //return sizeof(cmd_table) / sizeof(fun_desc_t) -2 ;
                break;
            }
            else {
                leng=0;
                bzero(path,1024);
            }
        } else {
            path[leng] = ch;
            leng++;
        }
    }
    
    //printf("2 %s\n", path);
    
    int pid, status;
    if((pid=fork())<0){
        perror("error fork: ");
        exit(1);
    }
    if(pid==0){
        leng = tokens_get_length(tokens);
        //printf("leng %d\n", leng);
        char *cmds[leng+1];
        cmds[0] = malloc(strlen(path)+1);
        strcpy(cmds[0], path);
        
        for(size_t i=1; i<leng; i++){
            char *str = tokens_get_token(tokens, i);
            cmds[i] = malloc(strlen(str)+1);
            strcpy(cmds[i], str);
            //printf("%s\n", cmds[i]);
        }
        cmds[leng]= 0;
        execv(cmds[0], cmds);
        perror("error: ");
        exit(0);
    }
    wait(&status);
    return 0;
}

/* Prints a helpful description for the given command */
int cmd_help(unused struct tokens *tokens) {
  for (unsigned int i = 0; i < sizeof(cmd_table) / sizeof(fun_desc_t); i++)
    printf("%s - %s\n", cmd_table[i].cmd, cmd_table[i].doc);
  return 1;
}

/* Exits this shell */
int cmd_exit(unused struct tokens *tokens) {
  exit(0);
}

/* Print current working directory*/
int cmd_pwd(unused struct tokens *tokens){
    char cwd[1024];
    if(getcwd(cwd,sizeof(cwd))!=NULL){
        fprintf(stdout,"%s\n",cwd);
    } else {
        perror("getcwd() error\n");
    }
    return 0;
}

/* change directory */
int cmd_cd(struct tokens *tokens){
    char *path = tokens_get_token(tokens, 1);
    if(path == NULL) {
        return 0;
    }
    char cwd[1024];
    if(path[0]!='/'){
        if(getcwd(cwd,sizeof(cwd))!=NULL){
            strcat(cwd,"/");
            strcat(cwd, path);
            if(chdir(cwd)<0){
                fprintf(stderr,"No such directory: %s\n",cwd);
            }
        }else{
            perror("getcwd() error\n");
        }
    } else {
        if(chdir(path)<0){
            fprintf(stderr,"No such directory: %s\n",cwd);
        }
    }
    return 0;
}

/* use buildin fun*/
int cmd_buildin(struct tokens *tokens){
    int pid, status;
    if((pid=fork())<0){
        perror("error open process: ");
        exit(1);
    }
    if(pid==0){
        /*
        char *str = tokens_get_token(tokens, 0);
        printf("%s\n", str);
        str = tokens_get_token(tokens, 1);
        printf("%s\n", str);
         */
        int leng = tokens_get_length(tokens);
        //printf("leng %d\n", leng);
        char *cmd[leng+1];
        for(size_t i=0; i<leng; i++){
            char *str = tokens_get_token(tokens, i);
            cmd[i] = malloc(strlen(str)+1);
            strcpy(cmd[i], str);
            //printf("%s\n", cmd[i]);
        }
        cmd[leng]= 0;
        
        execv(cmd[0], cmd);
        perror("error: ");
        exit(0);
    }
    wait(&status);
    //printf("all done!\n");
    return 1;
}

/* Looks up the built-in command, if it exists. */
int lookup(char cmd[]) {
    if(strchr(cmd, '/')!=NULL) return (sizeof(cmd_table) / sizeof(fun_desc_t) -1);
    for (unsigned int i = 0; i < sizeof(cmd_table) / sizeof(fun_desc_t); i++){
        if (cmd && (strcmp(cmd_table[i].cmd, cmd) == 0)){
            return i;
        }
    }
    
    const char *s = getenv("PATH");
    //printf("Path: %s\n", s);
    char path[1024];
    bzero(path,1024);
    int n =0;
    for(int i=0; i<strlen(s); i++){
        char ch = s[i];
        if(ch==':'|| (i+1) == strlen(s)){
            //i++;
            strcat(path, "/");
            strcat(path,cmd);
            if(access(path,F_OK)==0) {
                //printf("%s\n", path);
                return sizeof(cmd_table) / sizeof(fun_desc_t) -2 ;
            }
            else {
                n=0;
                bzero(path,1024);
            }
        } else {
            path[n] = ch;
            n++;
        }
    }
    
  return -1;
}



/* Intialization procedures for this shell */
void init_shell() {
  /* Our shell is connected to standard input. */
  shell_terminal = STDIN_FILENO;

  /* Check if we are running interactively */
  shell_is_interactive = isatty(shell_terminal);

  if (shell_is_interactive) {
    /* If the shell is not currently in the foreground, we must pause the shell until it becomes a
     * foreground process. We use SIGTTIN to pause the shell. When the shell gets moved to the
     * foreground, we'll receive a SIGCONT. */
    while (tcgetpgrp(shell_terminal) != (shell_pgid = getpgrp()))
      kill(-shell_pgid, SIGTTIN);

    /* Saves the shell's process id */
    shell_pgid = getpid();

    /* Take control of the terminal */
    tcsetpgrp(shell_terminal, shell_pgid);

    /* Save the current termios to a variable, so it can be restored later. */
    tcgetattr(shell_terminal, &shell_tmodes);
  }
}

int main(unused int argc, unused char *argv[]) {
  init_shell();

  static char line[4096];
  int line_num = 0;

    
  /* Please only print shell prompts when standard input is not a tty */
  if (shell_is_interactive)
    fprintf(stdout, "%d: ", line_num);

  while (fgets(line, 4096, stdin)) {
    /* Split our line into words. */
    struct tokens *tokens = tokenize(line);
    
    /* Find which built-in function to run. */
    int fundex = lookup(tokens_get_token(tokens, 0));
      //printf("%d    %s\n", fundex,tokens_get_token(tokens, 0));

    if (fundex >= 0) {
      cmd_table[fundex].fun(tokens);
    } else {
      /* REPLACE this to run commands as programs. */
      fprintf(stdout, "This shell doesn't know how to run programs.\n");
    }

    if (shell_is_interactive)
      /* Please only print shell prompts when standard input is not a tty */
      fprintf(stdout, "%d: ", ++line_num);

    /* Clean up memory */
    tokens_destroy(tokens);
  }

    return 0;
}
