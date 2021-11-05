#pragma once
#include "utils.h"

typedef int ActionType;
#define AT_NONE 0
#define AT_MOVE 1
#define AT_DELETE 2
#define AT_UNDO 3
#define AT_REDO 4
#define AT_OVERRIDE 5
#define AT_PASTE 6

typedef int RepaintType;
#define RP_NONE 0
#define RP_ALL 1
#define RP_LINES 2
#define RP_LOWER 3
#define RP_UPPER 4


struct Edit {
    size_t undo_index;
    size_t start_row;
    ssize_t start_col;  // -1 means entire row modification
    char* old_content;
    String* new_content;
};
typedef struct Edit Edit;

struct Buffer {
    char* name;
    FILE* file;
    FILE* swapfile;
    Deque /*Edit* */ undo_buffer;
    Vector /*Edit* */ redo_buffer;
    ssize_t top_row;            // Index into lines array corresponding to the top corner
    size_t top_left_file_pos;   // TODO: update this...
    size_t last_pos;            // TODO: update this...
    ssize_t cursor_row;         // 0-indexed Y coordinate on screen
    ssize_t cursor_col;         // 0-indexed X coordinate on screen
    int natural_col;
    ssize_t undo_index;
    Vector/*char* */ lines; //TODO: Cache/load buffered
    ssize_t visual_row;     // visual mode anchor. -1 means inactive
    ssize_t visual_col;     // visual mode anchor. -1 means line mode
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

#define CP_LINE 0
#define CP_SPLIT 1
typedef int CopyType;
struct Copy {
    Vector/* String* */ data;
    CopyType cp_type;
};

typedef struct Copy Copy;
