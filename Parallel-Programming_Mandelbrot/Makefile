
all: mandel

mandel: mandel.o bitmap.o
	gcc mandel.o bitmap.o -o mandel -lpthread -lm

mandel.o: mandel.c
	gcc -Wall -lm -g -c mandel.c -o mandel.o

bitmap.o: bitmap.c
	gcc -Wall -lm -g -c bitmap.c -o bitmap.o

mandelmovie: mandelmovie.c
	gcc -Wall -lm -lpthread -o mandelmovie mandelmovie.c

clean:
	rm -f mandel.o bitmap.o mandel
