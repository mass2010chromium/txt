CC=gcc
CFLAGS=-ggdb -Wall

objects = structures/buffer.o editor/utils.o editor/editor.o structures/Deque.o structures/Vector.o structures/String.o editor/editor_actions.o structures/gap_buffer.o 

all: bin editor/main.o $(objects)
	gcc -DDEBUG -c -o editor/debugging.o editor/debugging.c
	gcc editor/main.o editor/debugging.o $(objects) -lm -DDEBUG -o bin/main

release: bin editor/main.o $(objects)
	gcc -c -o editor/debugging.o editor/debugging.c
	gcc editor/main.o editor/debugging.o $(objects) -lm -o bin/txt

.PHONY: editor
editor: all
	./bin/main

editor/editor_actions.o: editor/editor_actions.c $(wildcard editor/actions/*.c)

valgrind_test: _test
	valgrind --leak-check=full --show-leak-kinds=definite,indirect,possible bin/test

test: _test
	bin/test

.PHONY: _test
_test: bin $(objects)
	gcc -DDEBUG -c -o editor/debugging.o editor/debugging.c
	gcc tests/test.c editor/debugging.o $(objects) -lm -o bin/test -ggdb
	cp tests/testfile tests/scratchfile

bin:
	mkdir bin

.PHONY: clean
clean:
	rm -f editor/*.o structures/*.o bin/*

.PHONY: install
install: release
	sudo strip ./bin/txt -o /usr/local/bin/txt
	sudo ln -sfT /usr/local/bin/txt /usr/local/bin/t
