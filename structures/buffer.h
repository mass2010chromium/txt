#pragma once

#include <stdio.h>
#include <regex.h>

#include "../editor/utils.h"
#include "../common.h"

Edit* make_Insert(size_t undo, size_t start_row, size_t start_col, char* new_content);
/**
 * Takes ownership of old_content!!!
 */
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
 * Get buffer mode. For now only guaranteed to be accurate for visual/visual line.
 */
EditorMode Buffer_get_mode(Buffer* buf);
void Buffer_set_mode(Buffer* buf, EditorMode);
void Buffer_exit_visual(Buffer* buf);

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
 * Clip the context to the buffer's bounds.
 * Only touches jump entries.
 */
void Buffer_clip_context(Buffer* buf, EditorContext* ctx);

/**
 * Paste a copy operation at `start_row, start_col` of ctx.
 */
RepaintType Buffer_insert_copy(Buffer* buf, Copy* copy, EditorContext* ctx);

/**
 * Delete a range of chars (from start to end in the given object)
 */
RepaintType Buffer_delete_range(Buffer* buf, Copy* copy, EditorContext* range);

/**
 * Copy a range of chars (from start to end in the given object)
 * Expects a normalized range.
 * Slightly redundant with delete_range, but delete_range is optimized
 * to take ownership whenever possible.
 */
void Buffer_copy_range(Buffer* buf, Copy* copy, EditorContext* range);

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
 * Saves the location of the last undo in ctx jump entries
 */
int Buffer_undo(Buffer*, size_t undo_index, EditorContext* ctx);
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
int Buffer_find_str(Buffer*, EditorContext* ret, char*, bool cross_lines, bool direction);

/**
 * Helper for Buffer_find_str.
 * Searches for `str` at the passed `line_num`
 * in `buf`, starting at position `offset`.
 * An offset of -1 signifies to search the entire line when searching backwards.
 * Return codes and directional behavior are the same as in Buffer_find_str.
 */
int Buffer_find_str_inline(Buffer* buf, EditorContext* ctx, char* str, size_t line_num, ssize_t offset, bool direction);

size_t read_file_break_lines(Vector* ret, FILE* infile);

/**
 * Searches the current line in `buf` for `c`.
 * If the character is found, updates `ctx->jump_col`
 * with the found position and.
 * Searches forwards if direction is true, otherwise backwards.
 * Returns 0 on success and -1 on failure.
 */
int Buffer_search_char(Buffer* buf, EditorContext* ctx, char c, bool direction);

/**
 * Moves the cursor to the next "word".
 * If skip_punct is false, this is the next punctuation mark
 * present in the buffer that is identical to the character at
 * the cursor's starting position.
 *
 * Otherwise, the next word begins at the next alphanumeric
 * character following a whitespace character (not necessarily consecutive).
 */
int Buffer_skip_word(Buffer* buf, EditorContext* ctx, bool skip_punct);
