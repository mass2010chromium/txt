#pragma once
#include "utils.h"

struct Edit {
    size_t undo_index;
    size_t start_row;
    size_t start_col;
    char* old_content;
    String* new_content;
};
typedef struct Edit Edit;

struct Buffer {
    char* name;
    FILE* file;
    FILE* swapfile;
    Deque /*Edit* ?*/ undo_buffer;
    Vector /*Edit* ?*/ redo_buffer;
    ssize_t top_row;            // Index into lines array corresponding to the top corner
    size_t top_left_file_pos;   // TODO: update this...
    size_t last_pos;            // TODO: update this...
    int cursor_row;         // 0-indexed Y coordinate on screen
    int cursor_col;         // 0-indexed X coordinate on screen
    int natural_col;
    Vector/*char* */ lines; //TODO: Cache/load buffered
};
typedef struct Buffer Buffer;

struct EditorContext {
    size_t jump_row;
    size_t jump_col;
    size_t start_row;
    size_t start_col;
    int action;
    Buffer* buffer;
};

typedef struct EditorContext EditorContext;
