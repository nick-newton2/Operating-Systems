/*
Nicholas Newton
OS Project 1
copyit.c
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

//GLOBAL/DEFINE
//#define BUFF_SIZE 4096
#define BUFF_SIZE 8192

//HEADERS
void display_message(int s);

//MAIN
int main(int argc, char *argv[]) {
    int sFD, dFD, read_in;
    //int write_out;
    int bytes=0;
    char *buff[BUFF_SIZE];

    //check input
    if (argc != 3){
        printf("%s: Invalid number of arguments!\n", argv[0]);
        printf("usage: %s <sourcefile> <targetfile>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    //set up periodic message
    signal(SIGALRM, display_message);
    alarm(1); // Initial timeout setting

    //open the source file or exit with an error
    sFD= open(argv[1], O_RDONLY);
    if(sFD < 0){
        printf("Unable to open %s: %s\n",argv[1],strerror(errno));
        exit(EXIT_FAILURE);
    }
    
    //create the target file or exit with an error
    dFD= open(argv[2], O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
    if(dFD < 0){
        printf("Error opening file %s: %s\n", argv[2], strerror(errno));
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
                printf("Error reading file %s: %s\n", argv[1], strerror(errno));
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
                printf("Error in writing data to %s: %s\n",argv[2], strerror(errno));
            }
        }
    }


    //close both files
    if(read_in < 0)
		printf("Error in reading data from %s\n",argv[1]);
	if(close(sFD) < 0)
		printf("Error in closing file %s\n",argv[1]);
	if(close(dFD) < 0)
		printf("Error in closing file %s\n",argv[2]);

    //print success message
    printf("%s: Copied %d bytes from file %s to %s.\n", argv[0], bytes, argv[1], argv[2]);
    exit(EXIT_SUCCESS); 
}

//FUNCTIONS
void display_message(int s) {
    printf("copyit: still copying...\n" );
    alarm(1);    //for every second
    signal(SIGALRM, display_message);
}