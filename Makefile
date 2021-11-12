CC=gcc
CFLAGS=-ggdb -Wall

objects = structures/buffer.o editor/utils.o editor/editor.o editor/debugging.o structures/Deque.o structures/Vector.o structures/String.o editor/editor_actions.o structures/gap_buffer.o 

all: bin editor/main.o $(objects)
	gcc editor/main.o $(objects) -o bin/main

.PHONY: editor
editor: all
	./bin/main

valgrind_test: _test
	valgrind --leak-check=full --show-leak-kinds=definite,indirect,possible bin/test

test: _test
	bin/test

.PHONY: _test
_test: bin $(objects)
	gcc tests/test.c $(objects) -o bin/test -ggdb
	cp tests/testfile tests/scratchfile

bin:
	mkdir bin

.PHONY: clean
clean:
	rm -f editor/*.o structures/*.o bin/*

.PHONY: install
install: all
	sudo strip ./bin/main -o /usr/local/bin/txt
