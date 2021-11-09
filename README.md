# txt: simple command line text editor

Built to run on linux with no dependencies. Seems to die on mac, unfortunate.

Like vim I guess. Command line text editor.

Current features:
- create file
- edit and save file
- Vim commands (d, A, i, w, q, hjkl, gg, dd, G, o, $, probably a few more I forgot)
    - see `editor_actions.c`
- Preserve indent
    - probably dies on some edge cases
- Enter visual mode (single line) by pressing `w`, or multi-line by pressing `W`.
- Search for a character inline by pressing `f` (forwards) or `F` (backwards), then the character to search for. `NORMAL` mode only.
- Search for words across lines by pressing `/` (forwards) or `?` (backwards), then the text to search for followed by `ENTER`. `NORMAL` mode only.

TODOS (major milestones):
- more unit test
- line wrapping
- non ascii char (multi space char, <1 space char)
