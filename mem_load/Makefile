ERROR_FLAGS = -std=gnu99 -Wall -Werror
CFLAGS = $(ERROR_FLAGS) -g -O0 -I../mem_alloc

cache_tests: mem_load.c
	gcc $(CFLAGS) -c mem_load.c
	gcc -o mem_load mem_load.o ../mem_alloc/mem_alloc.o -lm -lnuma
clean:
	rm -f *.o mem_load
