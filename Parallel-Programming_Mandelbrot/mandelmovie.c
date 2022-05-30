#include <getopt.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <errno.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

int main( int argc, char *argv[]){
	//Usage
	if (argc != 2) {
		printf("mandelmovie: Incorrect arguments\n");
		printf("Usage: %s <n processes>\n", argv[0]);
		exit(1);
	}

	//initialize
    int iter = 0;
	int n_proc = atoi(argv[1]);
	float s = 2; //initial scale

    //50 iters
	while (iter < 50) {
        // do n processes
		for (int i = 0; i < n_proc; i++) {
			if (iter < 50) {
				iter++;
				s = s / 1.2;
				int rc = fork();
                //fork failed; exit
				if (rc < 0) {
					fprintf(stderr, "mandelmovie: fork failed\n");
					exit(1);
				}
                // else if: child process
				else if(rc == 0) {
					char comm[256];
					char *args[100];
					sprintf(comm, "./mandel -x -0.57 -y -0.5 -m 200 -W 2000 -H 2000 -s %lf -o nnewton2_mandel%d.bmp", s, iter);
					char *tok;
					int words = 0;
					tok = strtok(comm, " ");
					while (tok != NULL) {
						args[words] = tok;
						tok = strtok(NULL, " ");
						words++;
					}
					args[words] = NULL;
					execvp(args[0], args);
				}
			}
		}

		for (int j = 0; j < n_proc; j++) {
			wait(NULL);
		}
		
	}
	return 0;
}



