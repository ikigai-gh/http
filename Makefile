CFLAGS = -g -std=c99 -Wall -Wextra -Wpedantic

all: server
server: server.o utils.o

clean:
	find -executable -type f -delete && rm *.o
run:
	./server 127.0.0.1 3000

.PHONY: clean run
