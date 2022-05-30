/*
Nicholas Newton
OS Project 2
myshell.c
*/

//LIBRARIES
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <signal.h>
#include <dirent.h>
#include <ctype.h>
#include <sys/wait.h>

//GLOBALS
#define BUFF_SIZE 4096
#define LEN 100

//FUNCTION PROTOTYPES
void get_input(char *);
int parse_words(char *, char **, char*, char*);
void decide(char **, int, char*, char*);
void start_func(char **);
void wait_func();
void run_func(char **);
void kill_func(char **);
void cont_func(char **);
void stop_func(char **);
void start_io(char **, int, char*, char*);
void run_io(char **, int, char*, char*);

//MAIN
int main(int argc, char *argv[]) {
    //check executable call
    if (argc != 1){
        printf("%s: Too many arguments!\n", argv[0]);
        printf("usage: %s\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    //loop shell
    char buf[BUFF_SIZE];
    char *words[LEN];
    char *input_len="";
    char *output_len="";
    while(1){
        //input
        get_input(buf);
        //parse words
        int io=0;
        io= parse_words(buf, words, input_len, output_len);
        //run command
        decide(words, io, input_len, output_len);
    }
    exit(EXIT_SUCCESS); 
}

//FUNCTIONS
void get_input(char * buf){
    //sleep(1);
    printf("myshell> ");
    fflush(stdout);
    fgets(buf, BUFF_SIZE, stdin);
    if (feof(stdin)){
        exit(0);
    }
}

int parse_words(char * buf, char ** words, char *input_len, char* output_len){
    int i=1;
    int io=0;
    char* token = strtok(buf, " \t\n");
    words[0]=token;
    if (token == NULL){
        words[0]="nULL";
    }
    while (token != NULL) {
        token = strtok(NULL, " \t\n");
        
        if (words[i][0]=='<'){
            input_len=words[i];
            io++; //infile
        }
        else if (words[i][0]=='>'){
            output_len=words[i];
            io+=2; //output_len
        }
        else{
            words[i]=token;
            i++;
        }
        //if io =0 nothing, =1 input, =2 output, =3 both

        if (i >LEN){
            printf("myshell: Error, too many arguments\n");
            exit(-1);
        }
    }
    words[i]=0;
    return io;
}

void decide(char ** words, int io, char* input_len, char* output_len){
    if (strcmp(words[0], "start")==0){
        if (io>0){
            start_io(words, io, input_len, output_len);
        }
        else{
            start_func(words);
        }
    }
    else if (strcmp(words[0], "run")==0){
        if (io>0){
            run_io(words, io, input_len, output_len);
        }
        else{
            run_func(words);
        }
    }
    else if (strcmp(words[0], "wait")==0){
        wait_func();
    }
    else if (strcmp(words[0], "kill")==0){
        kill_func(words);
    }
    else if (strcmp(words[0], "continue")==0){
        cont_func(words);
    }
    else if (strcmp(words[0], "stop")==0){
        stop_func(words);
    }
    else if (strcmp(words[0], "quit")==0 || strcmp(words[0], "exit")==0){
        exit(0);
    }
    else if (strcmp(words[0], "nULL")==0 ){
        //printf("fail");
    }
    else{
        printf("myshell: unknown command: %s\n", words[0]);
    }
}

void start_func(char ** words){
    int retval = fork();
    if (retval < 0){
        printf("Error: Fork failed\n");
        return;
    }
    else if (retval == 0) {
        // This is the child process
        execvp(words[1], &words[1]);
        // exec does not return if it succeeds
        printf("Error: Could not execute %s\n", words[0]);
        exit(1);
    } 
    printf("myshell: process %d started\n", retval);
}

void wait_func(){
        int status;
        int retval=wait(&status);
        int pid = retval;
        if (pid ==-1){
            printf("myshell: no processes left\n");
        }
        else if (WIFEXITED(status)){
            printf("Process %d exited normally with status: %d\n", retval, WEXITSTATUS(status));
        }
        else if (WIFSIGNALED(status)){
            printf("Process %d killed by signal: %d\n", retval, WTERMSIG(status));
        }
        else if (WIFSTOPPED(status)){
            printf("Process %d stopped by signal: %d\n", retval, WIFSTOPPED(status));
        }
        else if (WIFCONTINUED(status)){
            printf("Process %d continued\n", retval);
        }
        else{
            printf("Process %d exited abnormally with a status of %d\n", retval, status);
        }
}

void run_func(char ** words){
    int retval = fork();
    int status;
    if (retval < 0){
        printf("Error: Fork failed\n");
        return;
    }
    else if (retval == 0) {
        // This is the child process
        execvp(words[1], &words[1]);
        // exec does not return if it succeeds
        printf("Error: Could not execute %s\n", words[0]);
        exit(1);
    }

    int pid = retval;
    waitpid(pid, &status, 0);
    if (pid ==-1){
        printf("myshell: no processes left\n");
    }
    else if (WIFEXITED(status)){
        printf("Process %d exited normally with status: %d\n", retval, WEXITSTATUS(status));
    }
    else if (WIFSIGNALED(status)){
        printf("Process %d killed by signal: %d\n", retval, WTERMSIG(status));
    }
    else if (WIFSTOPPED(status)){
        printf("Process %d stopped by signal: %d\n", retval, WIFSTOPPED(status));
    }
    else if (WIFCONTINUED(status)){
        printf("Process %d continued\n", retval);
    }
    else{
        printf("Process %d exited abnormally with a status of %d\n", retval, status);
    }
}

void kill_func(char **words){
    if (words[1]==NULL) {
        printf("Error: Please provide PID\n");
        return;
    }
    int pid=atoi(words[1]);
    int code= kill(pid, SIGKILL);
    if (code ==0){
        printf("myshell: process %d killed\n", pid);
    }
    else{
        printf("Error: kill error: %s\n", strerror(errno));
    }
}

void cont_func(char **words){
    if (words[1]==NULL) {
        printf("Error: Please provide PID\n");
        return;
    }
    int pid=atoi(words[1]);
    int code= kill(pid, SIGCONT);
    printf("code:%d\n", code);
    if (code ==0){
        printf("myshell: process %d continued\n", pid);
    }
    else{
        printf("Error: continue error: %s\n", strerror(errno));
    }
}

void stop_func(char **words){
    if (words[1]==NULL) {
        printf("Error: Please provide PID\n");
        return;
    }
    int pid=atoi(words[1]);
    int code= kill(pid, SIGSTOP);
    if (code ==0){
        printf("myshell: process %d stopped\n", pid);
    }
    else{
        printf("Error: stop error: %s\n", strerror(errno));
    }
}





//add file stuff

void start_io(char ** words, int io, char* input_len, char* output_len){
    int retval = fork();
    int sFD, dFD;
    if (retval < 0){
        printf("Error: Fork failed\n");
        return;
    }
    else if (retval == 0) {
        switch(io){
            case 1:
                //infile

                //
                sFD= open(input_len, O_RDONLY);
                if(sFD < 0){
                    printf("Unable to open %s: %s\n",input_len,strerror(errno));
                    exit(EXIT_FAILURE);
                }
                dup2(sFD, STDIN_FILENO);
                //
                execvp(words[1], &words[1]);
                printf("Error: Could not execute %s\n", words[0]);
                exit(1);
                break;
            case 2:
                //output_len
                dFD= open(output_len, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
                if(dFD < 0){
                    printf("Error opening file %s: %s\n", output_len, strerror(errno));
                    exit(EXIT_FAILURE);
                }
                dup2(dFD, STDOUT_FILENO);
                execvp(words[1], &words[1]);
                printf("Error: Could not execute %s\n", words[0]);
                exit(1);
                break;
            case 3:
                //both
                //input_len
                //
                sFD= open(input_len, O_RDONLY);
                if(sFD < 0){
                    printf("Unable to open %s: %s\n",input_len,strerror(errno));
                    exit(EXIT_FAILURE);
                }
                dup2(sFD, STDIN_FILENO);
                dFD= open(output_len, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
                if(dFD < 0){
                    printf("Error opening file %s: %s\n", output_len, strerror(errno));
                    exit(EXIT_FAILURE);
                }
                dup2(dFD, STDOUT_FILENO);
                execvp(words[1], &words[1]);
                printf("Error: Could not execute %s\n", words[0]);
                exit(1);
                break;
        }
    } 
    printf("myshell: process %d started\n", retval);
}

void run_io(char ** words, int io, char* input_len, char* output_len){
    int status;
    int retval = fork();
    int sFD, dFD;
    if (retval < 0){
        printf("Error: Fork failed\n");
        return;
    }
    else if (retval == 0) {
        switch(io){
            case 1:
                //infile
                //
                sFD= open(input_len, O_RDONLY);
                if(sFD < 0){
                    printf("Unable to open %s: %s\n",input_len,strerror(errno));
                    exit(EXIT_FAILURE);
                }
                dup2(sFD, STDIN_FILENO);
                //
                execvp(words[1], &words[1]);
                printf("Error: Could not execute %s\n", words[0]);
                exit(1);
                break;
            case 2:
                //output_len
                dFD= open(output_len, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
                if(dFD < 0){
                    printf("Error opening file %s: %s\n", output_len, strerror(errno));
                    exit(EXIT_FAILURE);
                }
                dup2(dFD, STDOUT_FILENO);
                execvp(words[1], &words[1]);
                printf("Error: Could not execute %s\n", words[0]);
                exit(1);
                break;
            case 3:
                //both
                //infile
                //
                sFD= open(input_len, O_RDONLY);
                if(sFD < 0){
                    printf("Unable to open %s: %s\n",input_len,strerror(errno));
                    exit(EXIT_FAILURE);
                }
                dup2(sFD, STDIN_FILENO);
                dFD= open(output_len, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
                if(dFD < 0){
                    printf("Error opening file %s: %s\n", output_len, strerror(errno));
                    exit(EXIT_FAILURE);
                }
                dup2(dFD, STDOUT_FILENO);
                execvp(words[1], &words[1]);
                printf("Error: Could not execute %s\n", words[0]);
                exit(1);
                break;
        }
    }

    int pid = retval;
    waitpid(pid, &status, 0);
    if (pid ==-1){
        printf("myshell: no processes left\n");
    }
    else if (WIFEXITED(status)){
        printf("Process %d exited normally with status: %d\n", retval, WEXITSTATUS(status));
    }
    else if (WIFSIGNALED(status)){
        printf("Process %d killed by signal: %d\n", retval, WTERMSIG(status));
    }
    else if (WIFSTOPPED(status)){
        printf("Process %d stopped by signal: %d\n", retval, WIFSTOPPED(status));
    }
    else if (WIFCONTINUED(status)){
        printf("Process %d continued\n", retval);
    }
    else{
        printf("Process %d exited abnormally with a status of %d\n", retval, status);
    }
}