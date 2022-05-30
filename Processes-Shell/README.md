## Programs
The myshell.c is capable of executing, managing, and monitoring user level programs
This program will be similar in purpose and design to everyday shells like bash or tcsh, although the syntax will be slightly different
myshell will be invoked without any arguments, and will support several different commands

```sh
./myshell
myshell> start cp data.txt copy.txt
myshell: process 346 started
myshell> 
```

Included commands:
* start
* wait
* run
* stop
* continue
* kill
* stop
* continue
* quit/exit/EOF

```sh
yshell> wait
myshell: process 346 exited normally with status 0

myshell> wait
myshell: process 347 exited abnormally with signal 11: Segmentation fault.

myshell> wait
myshell: no processes left

myshell> run date
Mon Jan 19 11:51:57 EST 2009
myshell: process 348 exited normally with status 0

myshell> kill 349
myshell: process 349 killed

myshell> wait
myshell: process 349 exited abnormally with signal 9: Killed.

myshell> stop 350
myshell: process 350 stopped.

myshell> continue 350
myshell: process 350 continued
```


myshell_extracredit.c includes the general format for redirection of IO from stdin/stdout to files
```sh
start sort <infile >outfile
```
sort will use infile as its standard input file and outfile as its standard input file

## Testing
Create some simple programs that crash or exit with values other than zero, to make sure that wait and run report the correct exit status
Try running interactive program like vi, and use stop, continue, and kill on it to see what happens
