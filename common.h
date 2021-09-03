#pragma once
#include "utils.h"

typedef int ActionType;
extern const ActionType AT_NONE;
extern const ActionType AT_MOVE;
extern const ActionType AT_DELETE;
extern const ActionType AT_UNDO;
extern const ActionType AT_REDO;
extern const ActionType AT_OVERRIDE;

typedef int RepaintType;
extern const RepaintType RP_NONE;
extern const RepaintType RP_ALL;
extern const RepaintType RP_LINES;
extern const RepaintType RP_LOWER;
extern const RepaintType RP_UPPER;

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
    ssize_t cursor_row;         // 0-indexed Y coordinate on screen
    ssize_t cursor_col;         // 0-indexed X coordinate on screen
    int natural_col;
    ssize_t undo_index;
    Vector/*char* */ lines; //TODO: Cache/load buffered
};
typedef struct Buffer Buffer;

struct EditorContext {
    ssize_t jump_row;
    ssize_t jump_col;
    ssize_t start_row;
    ssize_t start_col;
    ssize_t undo_idx;
    ActionType action;
    Buffer* buffer;
};

typedef struct EditorContext EditorContext;

struct Copy {
    String* data;
};

typedef struct Copy Copy;
