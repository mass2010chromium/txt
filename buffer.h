#pragma once
#include <stdio.h>

#include "utils.h"

typedef int EditorActionType;
int ED_INSERT;
int ED_DELETE;
int ED_FILE;

struct EditorAction {
    size_t start_idx;
    size_t size;
    size_t index_in_parent;
    struct EditorAction* parent;
    Vector /*EditorAction*/ nested;
    EditorActionType type;
    union {
        char* buf;
        FILE* file;
    };
};
typedef struct EditorAction EditorAction;

struct EditorContext {
    size_t index;
    EditorAction* action;
};
typedef struct EditorContext EditorContext;

struct Buffer {
    char* name;
    FILE* file;
    FILE* swapfile;
    Deque /*EditorAction* ?*/ undo_buffer;
    size_t top_left_file_pos;
    size_t last_pos;
    EditorAction root;
    int cursor_row;
    int cursor_col;
    int natural_col;
    EditorAction* active_edit;
    Vector/*char* */ lines; //TODO: Cache/load buffered
};
typedef struct Buffer Buffer;

EditorAction* make_EditorAction(size_t start, size_t size, EditorActionType type);

void inplace_make_EditorAction(EditorAction* act, size_t start, size_t size,
                                    EditorActionType type);

void EditorAction_add_child(EditorAction* parent, EditorAction* child);
void EditorAction_destroy(EditorAction*);

Buffer* make_Buffer(char* filename);
void inplace_make_Buffer(Buffer*, char*);
void Buffer_destroy(Buffer*);

/**
 * Given a 'tree' of EditorActions, find where the corresponding index into the entire
 * "flattened tree" would land.
 */
void resolve_index(EditorAction* parent, size_t index, EditorContext* ret);

/**
 * Resolve the editor actions and write the result to a file.
 * Return 0 on success.
 */
int write_actions(FILE* out, EditorAction* parent, size_t* n_char);

int write_actions_string(char* out, EditorAction* parent, size_t* n_char);

size_t read_file_break_lines(Vector* ret, FILE* infile);
