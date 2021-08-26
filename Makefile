CC=gcc
CFLAGS=-ggdb -Wall

all: main.o buffer.o utils.o editor.o debugging.o Deque.o Vector.o String.o editor_actions.o
	gcc main.o buffer.o utils.o editor.o debugging.o Deque.o Vector.o String.o editor_actions.o -o main

test: test.o buffer.o utils.o editor.o
	gcc test.o buffer.o utils.o editor.o -o test

clean:
	rm -f main.o buffer.o utils.o test.o editor.o debugging.o Deque.o Vector.o String.o editor_actions.o
