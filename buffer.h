#pragma once
#include <stdio.h>

#include "utils.h"

struct EditorAction {
    size_t undo_index;
    size_t start_idx;
    char* old_content;
    char* new_content;
};
typedef struct EditorAction EditorAction;

EditorAction* make_EditorAction(size_t undo, size_t start, char* old_content);
void inplace_make_EditorAction(EditorAction*, size_t, size_t, char*);
void EditorAction_destroy(EditorAction*);

struct Buffer {
    char* name;
    FILE* file;
    FILE* swapfile;
    Deque /*EditorAction* ?*/ undo_buffer;
    ssize_t top_row;
    size_t top_left_file_pos;
    size_t last_pos;
    int cursor_row;
    int cursor_col;
    int natural_col;
    Vector/*char* */ lines; //TODO: Cache/load buffered
};
typedef struct Buffer Buffer;

Buffer* make_Buffer(const char* filename);
void inplace_make_Buffer(Buffer*, const char*);
void Buffer_destroy(Buffer*);

/**
 * Scroll by up to `amount` (signed). Positive is down.
 * Return: The actual amount scrolled
 */
ssize_t Buffer_scroll(Buffer* buf, size_t window_height, ssize_t amount);

size_t Buffer_get_line_index(Buffer* buf, size_t y);
char** Buffer_get_line(Buffer* buf, size_t y);

int Buffer_save(Buffer* buf);

size_t read_file_break_lines(Vector* ret, FILE* infile);

