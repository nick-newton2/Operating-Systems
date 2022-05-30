/*
Nicholas Newton
OS Project 1
copyit_extracredit.c
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

//GLOBALS
#define BUFF_SIZE 8192

//FUNCTION PROTOTYPES
void display_message(int s);
int copyf (char *source, char *target);
int copydir(char *source, char *target);

//MAIN
int main(int argc, char *argv[]) {
    //check input
    if (argc != 3){
        printf("%s: Invalid number of arguments!\n", argv[0]);
        printf("usage: %s <sourcefile> <targetfile>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    //set up periodic message
    signal(SIGALRM, display_message);
    alarm(1); // Initial timeout setting

    char *source = argv[1];
    char *target = argv[2];

    //call function
    int total = copydir(source, target);

    //print success message
    printf("%s: Copied %d bytes from file %s to %s.\n", argv[0], total, source, target);
    exit(EXIT_SUCCESS); 
}

//FUNCTIONS
//1 second interrupt message
void display_message(int s) {
    printf("copyit: still copying...\n" );
    alarm(1);    //for every second
    signal(SIGALRM, display_message);
}

//from copyit file copy
int copyf (char *source, char *target){
    int sFD, dFD, read_in;
    int bytes=0;
    char *buff[BUFF_SIZE];

    //open the source file or exit with an error
    sFD= open(source, O_RDONLY);
    if(sFD < 0){
        printf("Unable to open %s: %s\n",source,strerror(errno));
        exit(EXIT_FAILURE);
    }
    //create the target file or exit with an error
    dFD= open(target, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
    if(dFD < 0){
        printf("Error opening file %s: %s\n", target, strerror(errno));
        exit(EXIT_FAILURE);
    }

    //loop
    while(1) {
        //read;
        read_in = read(sFD,buff,BUFF_SIZE);
        if (read_in==0){
            break;
        }
        else if (read_in==-1){
            if(errno == EINTR){
                read_in = read(sFD,buff,BUFF_SIZE);
            }
            else{
                printf("Error reading file %s: %s\n", source, strerror(errno));
                exit(EXIT_FAILURE);
            }    
        }
        bytes += read_in;
        //write
        if(write(dFD,buff,read_in) != read_in){
            if(errno == EINTR){
                write(dFD,buff,read_in);
            }
            else{
                printf("Error in writing data to %s: %s\n",target, strerror(errno));
            }
        }
    }
    //close both files
    if(read_in < 0){
		printf("Error in reading data from %s\n",source);
    }
	if(close(sFD) < 0) {
		printf("Error in closing file %s\n",source);
    }
	if(close(dFD) < 0){
		printf("Error in closing file %s\n",target);
    }
    return bytes;  
}

//copy dir recursively
int copydir(char *source, char *target){
    int total =0;

    //get path
    struct stat path;
    stat(source, &path);
    
    //check if dir and copy
    if S_ISDIR(path.st_mode){
        mkdir(target, 0700);
        DIR *d;
        struct dirent *dir;
        d=opendir(source);

        if(d){
            while((dir=readdir(d)) != NULL){
                //copy content of dirs, not . .., recrusively
                if ((strcmp(dir->d_name, ".") != 0) && (strcmp(dir->d_name, "..") != 0)){
                    char *resource = malloc(strlen(source)+strlen(dir->d_name)+2);
                    sprintf(resource, "%s/%s", source, dir->d_name);
                    char *retarget = malloc(strlen(target)+strlen(dir->d_name)+2);
                    sprintf(retarget, "%s/%s", target, dir->d_name);
                    //recursive call
                    total += copydir(resource, retarget);
                }
            }
            closedir(d);
        }
    }
    //copy the files by calling function from copyit.c
    else{
        total += copyf(source, target);
    }
    return total;
}