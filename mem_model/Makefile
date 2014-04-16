ERROR_FLAGS = -std=gnu99 -Wall -Werror
CFLAGS = $(ERROR_FLAGS) -O0

mem_model: mem_model.c
	gcc $(CFLAGS) -c mem_model.c
	gcc -o mem_model mem_model.o -lpthread

clean:
	rm -f *.o mem_model
