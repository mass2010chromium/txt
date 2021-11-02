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

TODOS (major milestones):
- more unit test
- line wrapping
- visual mode
- non ascii char (multi space char, <1 space char)
