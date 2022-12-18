all: c-prog

c-prog: main.o
	$(CC) -g -no-pie $^ -o $@

main.o: main.c
	$(CC) -g -O -Wall -Wextra -Wpedantic -std=c11 -c $^

clean:
	rm -f c-prog *.o core

PHONY: clean
