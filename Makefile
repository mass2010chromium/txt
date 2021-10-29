CC=gcc
CFLAGS=-ggdb -Wall

all: main.o buffer.o utils.o editor.o debugging.o Deque.o Vector.o String.o editor_actions.o
	gcc main.o buffer.o utils.o editor.o debugging.o Deque.o Vector.o String.o editor_actions.o -o main

valgrind_test: _test
	valgrind --leak-check=full ./test

test: _test
	./test

.PHONY: _test
_test: buffer.o utils.o editor.o debugging.o Deque.o Vector.o String.o editor_actions.o
	gcc tests/test.c buffer.o utils.o editor.o debugging.o Deque.o Vector.o String.o editor_actions.o -o test -ggdb

clean:
	rm -f main.o buffer.o utils.o test.o editor.o debugging.o Deque.o Vector.o String.o editor_actions.o
