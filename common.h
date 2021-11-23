#pragma once
#include "editor/utils.h"

typedef int EditorMode;
#define EM_QUIT         -1
#define EM_NORMAL       0
#define EM_INSERT       1
#define EM_COMMAND      2
#define EM_VISUAL       3
#define EM_VISUAL_LINE  4

#define size_t unsigned long int

extern const char* EDITOR_MODE_STR[5];

typedef int ActionType;
#define AT_NONE 0
#define AT_MOVE 1
#define AT_DELETE 2
#define AT_UNDO 3
#define AT_REDO 4
#define AT_OVERRIDE 5
#define AT_PASTE 6
#define AT_ESCAPE 7
#define AT_COMMAND 8

typedef int RepaintType;
#define RP_NONE 0
#define RP_ALL 1
#define RP_LINES 2
#define RP_LOWER 3
#define RP_UPPER 4


/**
 * Struct an edit to a line. TODO: bulk/group edits.
 */
struct Edit {
    size_t undo_index;
    size_t start_row;
    ssize_t start_col;  // -1 means entire row modification
    char* old_content;
    String* new_content;
};
typedef struct Edit Edit;

/**
 * Struct representing a keypress. For macro recording and the like.
 */
struct Keystroke {
    char c;
    int control;
};
typedef struct Keystroke Keystroke;

/**
 * Struct representing a marked point in the text. TODO: assoc with lines, not rows.
 */
struct Mark {
    ssize_t row;
    ssize_t col;
    bool set;
};
typedef struct Mark Mark;

struct Buffer {
    String* name;
    String* swapfile_name;
    FILE* file;
    FILE* swapfile;
    Deque /*Edit* */ undo_buffer;
    Vector /*Edit* */ redo_buffer;
    ssize_t top_row;            // Index into lines array corresponding to the top corner
    ssize_t left_col;           // Column position of the leftmost column (default: 0)
    size_t top_left_file_pos;   // TODO: update this...
    size_t last_pos;            // TODO: update this...
    ssize_t cursor_row;         // 0-indexed Y coordinate on screen
    ssize_t cursor_col;         // 0-indexed X coordinate on screen
    int natural_col;            // This is int because.. if you have more than int cols, I can't save you
    ssize_t undo_index;
    Vector/*char* */ lines; //TODO: Cache/load buffered
    size_t visual_row;      // Visual mode anchors.
    size_t visual_col;
    EditorMode buffer_mode;
    Mark marks[256];
};
typedef struct Buffer Buffer;

struct EditorContext {
    ssize_t jump_row;
    ssize_t jump_col;
    ssize_t start_row;
    ssize_t start_col;
    ssize_t undo_idx;
    bool sharp_move;
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

/**
 * "Normalize" a set of row, col pairs such that
 * (ret->start_row, ret->start_col) comes "before"
 * (ret->jump_row, ret->jump_col).
 *
 * If c1 or c2 is negative, both jump columns are set to -1.
 */
void normalize_context(EditorContext* ret, ssize_t r1, ssize_t c1,
                                           ssize_t r2, ssize_t c2);
void EditorContext_normalize(EditorContext* self);
