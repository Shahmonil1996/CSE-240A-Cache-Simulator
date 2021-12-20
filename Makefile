CC=gcc
OPTS=-g -std=c99 -Werror

all: main.o 
	$(CC) $(OPTS) -lm -o main main.o 

main.o: main.c 
	$(CC) $(OPTS) -c main.c

clean:
	rm -f *.o main;
