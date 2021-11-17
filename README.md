# txt: simple command line text editor

Built to run on linux with no dependencies. Seems to die on mac, unfortunate.

Like vim I guess. Command line text editor.

## Current features:
- create file
- edit and save file
- Vim commands (d, A, i, w, q, hjkl, gg, dd, G, o, $, probably a few more I forgot)
    - see `editor_actions.c`
- Line macro and macro recording
    - `:norm` repeat typed commands on visual selection row by row
    - `q` record commands as they are typed, and play back
- Preserve indent
    - probably dies on some edge cases
- Enter visual mode (single line) by pressing `v`, or multi-line by pressing `V`.
- Search for a character inline by pressing `f` (forwards) or `F` (backwards),
  then the character to search for. `NORMAL` mode only.
- Search for words across lines by pressing `/` (forwards) or `?` (backwards),
  then the text to search for followed by `ENTER`. `NORMAL` mode only.

## Building `txt`

Requirements: gcc 9.3, valgrind, POSIX OS

- `make` or `make all`: Builds the editor, in `./bin/main`
- `make test`: Run test cases
- `make valgrind_test`: Run test cases, with valgrind
    - Note: The editor uses design patterns that result in "still reachable" memory, that is OK
- `sudo make install`: Copy binary to `/usr/local/bin`
    - lol it works on my machine

## Repo TL;DR

- `tests`: test files/testplan
- `editor`: editor specific stuff
- `structures`: generic data structures (kinda? buffer is in here for now, might move it)
- `bin`: binaries (test, main) go here

## Repository Branches

- weekX-dev: Used to view diffs between week 1/2/3 and main.
- main: up-to-date, working code. **Merge requests to main should be viewed for grading purposes.**
- jcp-dev: Jing's weekly development branch
- daniel-week-X: Daniel's weekly development branch.

## TODOS (major milestones):
- more unit test
- non ascii char (multi space char, <1 space char)
- Move options to config file (ex. line number, tab width, expandtab should be not be configured via code)
