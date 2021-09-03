#pragma once

#include <stdio.h>

#include "utils.h"
#include "common.h"

Edit* make_Insert(size_t undo, size_t start_row, size_t start_col, char* new_content);
Edit* make_Delete(size_t undo, size_t start_row, size_t start_col, char* old_content);
Edit* make_Edit(size_t undo, size_t start_row, size_t start_col, char* old_content);
void inplace_make_Edit(Edit*, size_t, size_t, size_t, char*);
void Edit_destroy(Edit*);

Buffer* make_Buffer(const char* filename);
void inplace_make_Buffer(Buffer*, const char*);
void Buffer_destroy(Buffer*);

/**
 * Scroll by up to `amount` (signed). Positive is down.
 * Return: The actual amount scrolled
 */
ssize_t Buffer_scroll(Buffer* buf, ssize_t window_height, ssize_t amount);

size_t Buffer_get_num_lines(Buffer* buf);

/**
 * These two get relative to screen pos.
 */
ssize_t Buffer_get_line_index(Buffer* buf, ssize_t y);
char** Buffer_get_line(Buffer* buf, ssize_t y);

/**
 * This gets relative to document pos.
 */
char** Buffer_get_line_abs(Buffer* buf, size_t row);

/**
 * Write a line in this buffer. Copies data into the buffer.
 */
void Buffer_set_line_abs(Buffer* buf, size_t row, const char* data);

/**
 * Paste a copy operation at `start_row, start_col` of ctx.
 */
RepaintType Buffer_insert_copy(Buffer* buf, Copy* copy, EditorContext* ctx);

/**
 * Delete a range of chars (from start to end in the given object)
 */
RepaintType Buffer_delete_range(Buffer* buf, Copy* copy, EditorContext* range);

int Buffer_save(Buffer* buf);

/**
 * Push an entry onto the undo buffer. This should be done for all changes to the buffer content
 */
void Buffer_push_undo(Buffer*, Edit*);

/*
 * Undo actions until the top of the undo buffer has an action index less than the specified undo index.
 * Undone actions are pushed onto the redo buffer.
 * Returns the number of actions undone. (Possibly zero)
 * Postcondition: rightmost element of undo buffer has action index < undo_index, or undo buffer is empty.
 */
int Buffer_undo(Buffer*, size_t undo_index);
int Buffer_redo(Buffer*, size_t undo_index);

/**
 * Find a string in this buffer.
 * Starts from the position given in the EditorContext struct (row, col)
 * Search forward if direction is true, else backwards
 * Search in more than the current line if cross_lines is true
 * If found, returns the row, col in the EditorContext struct
 *
 * Return: 0 = OK, 1 = NOT_FOUND, -1 = error
 */
int Buffer_find_str(Buffer*, char*, bool cross_lines, bool direction, EditorContext* ret);

size_t read_file_break_lines(Vector* ret, FILE* infile);
