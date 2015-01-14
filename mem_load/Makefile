ERROR_FLAGS = -std=gnu99 -Wall -Werror
CFLAGS = $(ERROR_FLAGS) -g -O0 -I../mem_alloc

mem_load: mem_load.o
	gcc $(CFLAGS) -c mem_load.c
	gcc -o mem_load mem_load.o ../mem_alloc/mem_alloc.o -lm -lnuma

mem_load.o: mem_load.s
	gcc $(CFLAGS) -c mem_load.s

mem_load.s: mem_load.c
	gcc $(CFLAGS) -S mem_load.c

clean:
	rm -f *.o *.s mem_load
