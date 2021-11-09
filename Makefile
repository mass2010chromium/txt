CC=gcc
CFLAGS=-ggdb -Wall

all: editor/main.o structures/buffer.o editor/utils.o editor/editor.o editor/debugging.o structures/Deque.o structures/Vector.o structures/String.o editor/editor_actions.o structures/gap_buffer.o
	gcc editor/main.o structures/buffer.o editor/utils.o editor/editor.o editor/debugging.o structures/Deque.o structures/Vector.o structures/String.o editor/editor_actions.o structures/gap_buffer.o -o bin/main

valgrind_test: _test
	valgrind --leak-check=full --show-leak-kinds=all bin/test

test: _test
	bin/test

.PHONY: _test
_test: structures/buffer.o editor/utils.o editor/editor.o editor/debugging.o structures/Deque.o structures/Vector.o structures/String.o editor/editor_actions.o structures/gap_buffer.o
	gcc tests/test.c structures/buffer.o editor/utils.o editor/editor.o editor/debugging.o structures/Deque.o structures/Vector.o structures/String.o editor/editor_actions.o structures/gap_buffer.o -o bin/test -ggdb
clean:
	rm -f editor/*.o structures/*.o bin/* main test buffertest
