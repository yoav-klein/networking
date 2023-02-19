

CC=gcc 
CFLAGS=-ansi -pedantic-errors -Wall -Wextra -g

server.out: server.c
	$(CC) $(CFLAGS) $^ -o $@
	

.PHONY: clean

clean:
	rm *.out	
