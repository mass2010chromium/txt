#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <sys/sendfile.h>

#include "buffer.h"
#include "../editor/utils.h"
#include "../editor/editor.h"

Edit* make_Insert(size_t undo, size_t start_row, size_t start_col, char* new_content) {
    Edit* ret = malloc(sizeof(Edit));
    ret->undo_index = undo;
    ret->start_row = start_row;
    ret->start_col = start_col;
    ret->old_content = NULL;
    ret->new_content = make_String(new_content);
    return ret;
}

/**
 * Takes ownership of old_content!!!
 */
Edit* make_Delete(size_t undo, size_t start_row, size_t start_col, char* old_content) {
    Edit* ret = malloc(sizeof(Edit));
    ret->undo_index = undo;
    ret->start_row = start_row;
    ret->start_col = start_col;
    ret->old_content = old_content;
    ret->new_content = NULL;
    return ret;
}

Edit* make_Edit(size_t undo, size_t start_row, size_t start_col, char* old_content) {
    Edit* ret = malloc(sizeof(Edit));
    inplace_make_Edit(ret, undo, start_row, start_col, old_content);
    return ret;
}

void inplace_make_Edit(Edit* ed, size_t undo,
                                size_t start_row, size_t start_col, char* old_content) {
    ed->undo_index = undo;
    ed->start_row = start_row;
    ed->start_col = start_col;
    ed->old_content = strdup(old_content);
    ed->new_content = make_String(old_content);
}

void Edit_destroy(Edit* ed) {
    free(ed->old_content);
    free(ed->new_content);
}

void Buffer_close_files(Buffer* buf) {
    if (buf->swapfile != NULL) {
        fclose(buf->swapfile);
        buf->swapfile = NULL;
    }
    if (buf->file != NULL) {
        fclose(buf->file);
        buf->file = NULL;
    }
}

Buffer* make_Buffer(const char* filename) {
    Buffer* ret = malloc(sizeof(Buffer));
    inplace_make_Buffer(ret, filename);
    return ret;
}

void inplace_make_Buffer(Buffer* buf, const char* filename) {
    // zero initialize fields by default.
    memset(buf, 0, sizeof(Buffer));

    inplace_make_Deque(&buf->undo_buffer, 100);
    inplace_make_Vector(&buf->redo_buffer, 100);
    inplace_make_Vector(&buf->lines, 100);
    if (filename == NULL) {
        filename = "__tmp__";
        Vector_push(&buf->lines, strdup(""));
    }
    else {
        FILE* infile = fopen(filename, "r+");
        // TODO use n_read
        size_t n_read;
        if (infile != NULL)  {
            //TODO buffer/read not the whole file
            n_read = read_file_break_lines(&buf->lines, infile);
            fclose(infile);
        }
        else {
            Vector_push(&buf->lines, strdup(""));
            n_read = 0;
        }
        (void) n_read;
    }
    buf->name = make_String(filename);
    buf->swapfile_name = Strdup(buf->name);
    Strcats(&buf->swapfile_name, ".swp");
    buf->buffer_mode = EM_NORMAL;
}
    
void Buffer_destroy(Buffer* buf) {
    size_t deque_idx = buf->undo_buffer.head;
    for (size_t i = 0; i < buf->undo_buffer.size; ++i) {
        Edit_destroy(buf->undo_buffer.elements[deque_idx]);
        free(buf->undo_buffer.elements[deque_idx]);
        ++deque_idx;
        if (deque_idx == buf->undo_buffer.max_size) {
            deque_idx = 0;
        }
    }
    Deque_destroy(&buf->undo_buffer);

    for (size_t i = 0; i < buf->redo_buffer.size; ++i) {
        Edit_destroy(buf->redo_buffer.elements[i]);
        free(buf->redo_buffer.elements[i]);
    }
    Vector_destroy(&buf->redo_buffer);

    for (size_t i = 0; i < buf->lines.size; ++i) {
        free(buf->lines.elements[i]);
    }
    Vector_destroy(&buf->lines);
    free(buf->name);
    free(buf->swapfile_name);
    Buffer_close_files(buf);
}

void Buffer_open_files(Buffer* buf, const char* mode_infile, const char* mode_swapfile) {
    if (mode_infile != NULL) { buf->file = fopen(buf->name->data, mode_infile); }
    if (mode_swapfile != NULL) {
        buf->swapfile = fopen(buf->swapfile_name->data, mode_swapfile);
    }
}


/**
 * Scroll by up to `amount` (signed). Positive is down.
 * Return: The actual amount scrolled
 */
ssize_t Buffer_scroll(Buffer* buf, ssize_t window_height, ssize_t amount) {
    ssize_t scroll_amount;
    if (-amount > buf->top_row) {
        scroll_amount = -buf->top_row;
        buf->top_row = 0;
        return scroll_amount;
    }
    else {
        ssize_t save = buf->top_row;
        buf->top_row += amount;
        if (buf->top_row + window_height > buf->lines.size) {
            scroll_amount = buf->lines.size - window_height - save;
            buf->top_row = buf->lines.size - window_height;
            if (buf->top_row < 0) buf->top_row = 0;
            return scroll_amount;
        }
        return amount;
    }
}

size_t Buffer_get_num_lines(Buffer* buf) {
    return buf->lines.size;
}

/**
 * Get buffer mode. For now only guaranteed to be accurate for visual/visual line.
 */
EditorMode Buffer_get_mode(Buffer* buf) {
    return buf->buffer_mode;
}
void Buffer_set_mode(Buffer* buf, EditorMode mode) {
    buf->buffer_mode = mode;
}
void Buffer_exit_visual(Buffer* buf) {
    Buffer_set_mode(buf, EM_NORMAL);

    EditorContext ctx;
    ctx.start_row = Buffer_get_line_index(buf, buf->cursor_row);
    ctx.start_col = buf->cursor_col;
    ctx.jump_row = buf->visual_row;
    ctx.jump_col = buf->visual_col;
    ctx.buffer = buf;
    EditorContext_normalize(&ctx);
    editor_repaint(RP_LINES, &ctx);
}

/**
 * These two get relative to screen pos.
 */
ssize_t Buffer_get_line_index(Buffer* buf, ssize_t y) {
    return y + buf->top_row;
}
char** Buffer_get_line(Buffer* buf, ssize_t y) {
    if (y + buf->top_row >= buf->lines.size) return NULL;
    return (char**) &(buf->lines.elements[y + buf->top_row]);
}

/**
 * This gets relative to document pos.
 */
char** Buffer_get_line_abs(Buffer* buf, size_t row) {
    return (char**) &(buf->lines.elements[row]);
}

/**
 * This gets relative to document pos.
 */
void Buffer_set_line_abs(Buffer* buf, size_t row, const char* data) {
    char** line_p = Buffer_get_line_abs(buf, row);
    free(*line_p);
    *line_p = strdup(data);
}

/**
 * Clip the context to the buffer's bounds.
 * Only touches jump entries.
 */
void Buffer_clip_context(Buffer* buf, EditorContext* ctx) {
    if (ctx->jump_col < 0) {
        ctx->jump_col = 0;
    }
    if (ctx->jump_row < 0) {
        ctx->jump_row = 0;
    }
    if (ctx->jump_row >= Buffer_get_num_lines(buf)) {
        ctx->jump_row = Buffer_get_num_lines(buf) - 1;
    }
    char* line = *Buffer_get_line_abs(buf, ctx->jump_row);
    size_t max_char = strlen(line);
    // Save newlines, but don't count them towards line length for cursor purposes.
    if (max_char > 0 && line[strlen(line) - 1] == '\n') {
        max_char -= 1;
    }
    if (ctx->jump_col > max_char) {
        ctx->jump_col = max_char;
    }
}

/**
 * Paste a copy operation at `start_row, start_col` of ctx.
 */
RepaintType Buffer_insert_copy(Buffer* buf, Copy* copy, EditorContext* ctx) {
    size_t n_lines = copy->data.size;
    if (n_lines == 0) return RP_NONE;
    size_t undo_idx = ctx->undo_idx;

    if (copy->cp_type == CP_LINE) {
        Vector_create_range(&buf->lines, ctx->start_row+1, n_lines);
        for (int i = 0; i < n_lines; ++i) {
            String* s = copy->data.elements[i];
            buf->lines.elements[ctx->start_row+1 + i] = strdup(s->data);
            Buffer_push_undo(buf, make_Insert(undo_idx, ctx->start_row+1 + i, -1, s->data));
        }
        return RP_LOWER;
    }
    else if (copy->cp_type == CP_SPLIT) {
        size_t n_newlines = n_lines - 1;
        char* line = buf->lines.elements[ctx->start_row];
        // Extra +1 to emulate vim's paste behavior (paste after).
        char* rest = line + ctx->start_col + 1;
        String* first = copy->data.elements[0];

        if (n_newlines == 0) {
            print("Case single line paste\n");
            size_t line_len = strlen(line);
            line = realloc(line, line_len + Strlen(first) + 1);
            rest = line + ctx->start_col + 1;

            // TODO: Use Strdup
            Edit* edit = make_Insert(undo_idx, ctx->start_row, ctx->start_col+1, first->data);
            Buffer_push_undo(buf, edit);

            memmove(rest + Strlen(first), rest,
                    line_len - ctx->start_col /* + 1 - 1 */);
            strncpy(rest, first->data, Strlen(first));
            buf->lines.elements[ctx->start_row] = line;
            // TODO signal lines for RP_LINES
            return RP_ALL;
        }

        Vector_create_range(&buf->lines, ctx->start_row+1, n_newlines);

        for (size_t i = 1; i < n_newlines; ++i) {
            String* s = copy->data.elements[i];
            buf->lines.elements[ctx->start_row + n_newlines] = strdup(s->data);
            Buffer_push_undo(buf, make_Insert(undo_idx, ctx->start_row + i, -1, s->data));
        }

        String* last_line_start = Strdup(copy->data.elements[n_newlines]);
        Strcats(&last_line_start, rest);
        Buffer_push_undo(buf, make_Insert(undo_idx, ctx->start_row + n_newlines, -1,
                                          last_line_start->data));
        buf->lines.elements[ctx->start_row + n_newlines] = String_to_cstr(last_line_start);

        // Fix the first line.
        line = realloc(line, ctx->start_col + Strlen(first) + 2);
        rest = line + ctx->start_col + 1;

        Edit* edit = make_Insert(undo_idx, ctx->start_row, ctx->start_col+1, first->data);
        edit->old_content = strdup(rest);

        Buffer_push_undo(buf, edit);
        strcpy(rest, first->data);
        buf->lines.elements[ctx->start_row] = line;
        print("realloc: %s %ld %lu\n", line, ctx->start_col, Strlen(first));

        // TODO signal lines for RP_LINES
        return RP_ALL;
    }
    return RP_NONE;
}

/**
 * Delete a range of chars (from start to end in the given object)
 * Expects a normalized range.
 */
RepaintType Buffer_delete_range(Buffer* buf, Copy* copy, EditorContext* range) {
    // delete forwards.
    for (int i = 0; i < copy->data.size; ++i) {
        free(copy->data.elements[i]);
    }
    Vector_clear(&copy->data, 10);
    ssize_t first_row = range->start_row;
    ssize_t last_row = range->jump_row;
    size_t undo_idx = range->undo_idx;
    if (range->start_col == -1) {   // Line delete mode
        copy->cp_type = CP_LINE;
        for (ssize_t i = first_row; i <= last_row; ++i) {
            char* line = *Buffer_get_line_abs(buf, i);
            Vector_push(&copy->data, make_String(line));

            // Ownership transfer (line)
            Buffer_push_undo(buf, make_Delete(undo_idx, first_row, -1, line));
        }
        Vector_delete_range(&buf->lines, first_row, last_row+1);
        if (buf->lines.size == 0) {
            Vector_push(&buf->lines, strdup(""));
        }
        return RP_LOWER;
    }

    copy->cp_type = CP_SPLIT;
    size_t start_c = range->start_col;
    size_t end_c = range->jump_col + 1;
    char** line_p = Buffer_get_line_abs(buf, first_row);
    char* old_content;

    if (last_row == first_row) {
        old_content = strndup(*line_p + start_c, end_c - start_c);
        Vector_push(&copy->data, make_String(old_content));
        Edit* edit = make_Delete(undo_idx, first_row, start_c, old_content);
        Buffer_push_undo(buf, edit);

        char* line = *line_p;
        size_t line_len = strlen(line);
        size_t rest = line_len - end_c;
        memmove(line + start_c, line + end_c, rest+1);
        return RP_LINES;
    }

    old_content = strdup(*line_p + start_c);
    Vector_push(&copy->data, make_String(old_content));
    Edit* modify_first_row = make_Delete(undo_idx, first_row, start_c, old_content);
    Buffer_push_undo(buf, modify_first_row);

    String* final_copy_add = NULL;
    if (last_row < Buffer_get_num_lines(buf)) {
        // Remove last row, merge with first row

        // Delete to jump col, inclusive.
        char* old_content = *Buffer_get_line_abs(buf, last_row);
        size_t target_len = strlen(old_content);
        size_t delete_to = range->jump_col + 1;
        if (target_len < delete_to) {
            delete_to = target_len;
        }
        final_copy_add = alloc_String(delete_to);
        Strncats(&final_copy_add, old_content, delete_to);

        // Ownership transfer.
        Edit* delete_last_row = make_Delete(undo_idx, last_row, -1, old_content);
        Buffer_push_undo(buf, delete_last_row);

        String* last_row_fragment = make_String(delete_last_row->old_content + delete_to);

        Vector_delete(&buf->lines, last_row);

        // Ownership transfer.
        modify_first_row->new_content = last_row_fragment;
        print("Save end: %ld %ld %s\n", target_len, delete_to, last_row_fragment->data);
    }
    size_t new_len = start_c + 1;
    if (modify_first_row->new_content != NULL) {
        new_len += Strlen(modify_first_row->new_content);
    }
    char* new_line = realloc(*line_p, new_len);
    new_line[start_c] = 0;
    strcat(new_line, modify_first_row->new_content->data);
    *line_p = new_line;

    if (last_row > first_row + 1) {
        for (size_t i = last_row - 1; i > first_row; --i) {
            char* line = *Buffer_get_line_abs(buf, i);
            Vector_push(&copy->data, make_String(line));

            // Ownership transfer (line)
            Buffer_push_undo(buf, make_Delete(undo_idx, i, -1, line));
        }
        Vector_delete_range(&buf->lines, first_row+1, last_row);
        if (buf->lines.size == 0) {
            Vector_push(&buf->lines, strdup(""));
        }
    }
    if (final_copy_add != NULL) {
        Vector_push(&copy->data, final_copy_add);
    }
    return RP_LOWER;
}

/**
 * Copy a range of chars (from start to end in the given object)
 * Expects a normalized range.
 * Slightly redundant with delete_range, but delete_range is optimized
 * to take ownership whenever possible.
 */
void Buffer_copy_range(Buffer* buf, Copy* copy, EditorContext* range) {
    for (int i = 0; i < copy->data.size; ++i) {
        free(copy->data.elements[i]);
    }
    Vector_clear(&copy->data, 10);
    ssize_t first_row = range->start_row;
    ssize_t last_row = range->jump_row;
    if (range->start_col == -1) {   // Line copy mode.
        copy->cp_type = CP_LINE;
        for (ssize_t i = first_row; i <= last_row; ++i) {
            char* line = *Buffer_get_line_abs(buf, i);
            Vector_push(&copy->data, make_String(line));
        }
        return;
    }

    copy->cp_type = CP_SPLIT;
    size_t start_c = range->start_col;
    size_t end_c = range->jump_col + 1;
    char** line_p = Buffer_get_line_abs(buf, first_row);

    if (last_row == first_row) {
        size_t copy_len = end_c - start_c;
        String* s = alloc_String(copy_len);
        Vector_push(&copy->data, Strncats(&s, *line_p + start_c, copy_len));
        return;
    }

    Vector_push(&copy->data, make_String(*line_p + start_c));

    if (last_row > first_row + 1) {
        for (size_t i = last_row - 1; i > first_row; --i) {
            char* line = *Buffer_get_line_abs(buf, i);
            Vector_push(&copy->data, make_String(line));
        }
    }

    if (last_row < Buffer_get_num_lines(buf)) {
        // Remove last row, merge with first row

        // Delete to jump col, inclusive.
        char* old_content = *Buffer_get_line_abs(buf, last_row);
        size_t target_len = strlen(old_content);
        size_t select_to = range->jump_col + 1;
        if (target_len < select_to) {
            select_to = target_len;
        }
        String* final_copy_add = alloc_String(select_to);
        Vector_push(&copy->data, Strncats(&final_copy_add, old_content, select_to));
    }
}

int Buffer_save(Buffer* buf) {
    Buffer_open_files(buf, NULL, "w");
    for (int i = 0; i < buf->lines.size; ++i) {
        // TODO check return value
        fwrite(buf->lines.elements[i], strlen(buf->lines.elements[i]), 1, buf->swapfile);
    }
    fseek(buf->swapfile, 0, SEEK_END);
    size_t bytes = ftell(buf->swapfile);
    Buffer_close_files(buf);
    Buffer_open_files(buf, "w", "r");
    while (bytes > 0) {
        ssize_t nbytes = sendfile(fileno(buf->file), fileno(buf->swapfile), NULL, bytes);
        if (nbytes < 0) {
            // TODO handle failure?
            Buffer_close_files(buf);
            return -1;
        }
        bytes -= nbytes;
    }
    Buffer_close_files(buf);
    remove(buf->swapfile_name->data);
    return 0;
}

void Buffer_rename(Buffer* buf, char* new_name) {
    String_clear(buf->name);
    String_clear(buf->swapfile_name);
    Strcats(&buf->name, new_name);
    Strcats(&buf->swapfile_name, new_name);
    Strcats(&buf->swapfile_name, ".swp");
}

/**
 * Push an entry onto the undo buffer. This should be done for all changes to the buffer content
 */
void Buffer_push_undo(Buffer* buf, Edit* ed) {
    if (ed->old_content == NULL && ed->new_content == NULL) {
        Edit_destroy(ed);
        free(ed);
        return;
    }
    if (Deque_full(&buf->undo_buffer)) {
        Edit* ed_old = Deque_pop(&buf->undo_buffer);
        Edit_destroy(ed_old);
        free(ed_old);
    }
    Deque_push(&buf->undo_buffer, ed);
    for (size_t i = 0; i < buf->redo_buffer.size; ++i) {
        Edit* redo_edit = (Edit*) buf->redo_buffer.elements[i];
        Edit_destroy(redo_edit);
        free(redo_edit);
    }
    Vector_clear(&buf->redo_buffer, 100);
}

/**
 * PRIVATE
 * Undo the application of a given edit to this buffer.
 */
void Buffer_undo_Edit(Buffer* buf, Edit* ed) {
    print("Undo edit: %ld, %ld\n", ed->start_row, ed->start_col);
    size_t index = ed->start_row;
    if (ed->old_content == NULL) {
        // Insert action. Undo by deleting.
        char* lineptr = buf->lines.elements[index];
        if (ed->start_col == -1) {  // Line insert
            Vector_delete(&buf->lines, index);
            free(lineptr);
            return;
        }
        size_t current_len = strlen(lineptr);
        size_t remove_len = Strlen(ed->new_content);
        size_t ending = current_len - ed->start_col - remove_len + 1; // +1 null byte
        memmove(lineptr + ed->start_col, lineptr + ed->start_col + remove_len, ending);
    }
    else if (ed->new_content == NULL) {
        // Delete action. Undo by inserting.
        if (ed->start_col == -1) {  // Line delete
            Vector_insert(&buf->lines, index, strdup(ed->old_content));
            return;
        }
        char* to_insert = ed->old_content;
        size_t insert_len = strlen(to_insert);
        char** lineptr = (char**) &(buf->lines.elements[index]);
        size_t current_len = strlen(*lineptr);
        *lineptr = realloc(*lineptr, current_len + insert_len + 1);
        size_t ending = current_len - ed->start_col + 1; // +1 null byte
        memmove(*lineptr + insert_len + ed->start_col, *lineptr + ed->start_col, ending);
        memcpy(*lineptr + ed->start_col, to_insert, insert_len);
    }
    else {
        char** lineptr = (char**) &(buf->lines.elements[index]);
        if (ed->start_col == -1) {  // Line replace
            free(*lineptr);
            *lineptr = strdup(ed->old_content);
            return;
        }
        size_t current_len = strlen(*lineptr);
        char* to_insert = ed->old_content;
        size_t insert_len = strlen(to_insert);
        String* to_remove = ed->new_content;
        size_t remove_len = Strlen(to_remove);
        if (insert_len > remove_len) {
            size_t line_size = current_len + insert_len - remove_len;
            *lineptr = realloc(*lineptr, line_size + 1);
        }
        size_t ending = current_len - ed->start_col - remove_len + 1; // +1 null byte
        memmove(*lineptr + insert_len + ed->start_col, *lineptr + remove_len + ed->start_col, ending);
        memcpy(*lineptr + ed->start_col, to_insert, insert_len);
    }
}

/*
 * Undo actions until the top of the undo buffer has an action index less than the specified undo index.
 * Undone actions are pushed onto the redo buffer.
 * Returns the number of actions undone. (Possibly zero)
 * Postcondition: rightmost element of undo buffer has action index < undo_index, or undo buffer is empty.
 * Saves the location of the last undo in ctx jump entries
 */
int Buffer_undo(Buffer* buf, size_t undo_index, EditorContext* ctx) {
    int num_undo = 0;
    while (true) {
        if (Deque_empty(&buf->undo_buffer)) {
            return num_undo;
        }
        Edit* ed = Deque_peek_r(&buf->undo_buffer);
        if (ed->undo_index < undo_index) {
            return num_undo;
        }
        num_undo += 1;
        ctx->jump_row = ed->start_row;
        ctx->jump_col = ed->start_col;
        Buffer_undo_Edit(buf, ed);
        Deque_pop_r(&buf->undo_buffer);
        Vector_push(&buf->redo_buffer, ed);
    }
}

//TODO implement
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
int Buffer_find_str(Buffer* buf, EditorContext* ctx, char* str, bool cross_lines, bool direction) {
    if (!cross_lines) {
        return Buffer_find_str_inline(buf, ctx, str, ctx->jump_row, ctx->jump_col, direction);
    }
    int boundary = Buffer_get_num_lines(buf);
    int offset = 1;
    if (!direction) {
        boundary = -1;
        offset = -1;
    }
    int col_offset = ctx->jump_col;
    for (size_t i = ctx->jump_row; i != boundary; i += offset) {
        int result = Buffer_find_str_inline(buf, ctx, str, i, col_offset, direction);
        if (result == 0) {
            return result;
        }
        col_offset = -1;
    }
    return -1;  // TODO
}

int Buffer_find_str_inline(Buffer* buf, EditorContext* ctx, char* str, size_t line_num, ssize_t offset, bool direction) {
    bool search_full = false;
    if (offset == -1) {
        search_full = true;
        offset = 0;
    }
    char* line = *(Buffer_get_line_abs(buf, line_num));
    regex_t regex;
    regmatch_t pmatch;
    int status = regcomp(&regex, str, 0);
    if (status != 0) {
        return -2;
    }
    size_t search_offset = 0;
    status = regexec(&regex, line, 1, &pmatch, 0);
    ssize_t pattern_loc = -1;
    ssize_t prev_loc = -1;
    while (status == 0) {
        pattern_loc = search_offset + pmatch.rm_so;
        if (search_full || (pattern_loc > prev_loc && pattern_loc < offset)) {
            prev_loc = pattern_loc;
        }
        if (direction) {
            if (search_full || pattern_loc > offset) {
                ctx->jump_col = pattern_loc;
                ctx->jump_row = line_num;
                regfree(&regex);
                return 0;
            }
        }
        search_offset += pmatch.rm_eo;
        status = regexec(&regex, line + search_offset, 1, &pmatch, REG_NOTBOL);
    }
    regfree(&regex);
    //After loop finishes, non-negative prev_loc indicates location of last match before cursor offset
    if (!direction) {
        if (prev_loc >= 0 && (search_full || prev_loc < offset)) {
            ctx->jump_col = prev_loc;
            ctx->jump_row = line_num;
            return 0;
        }
    }
    return -1;
}

/**
 * Read a file into a vector. One entry in the vector for each line in the file.
 * All strings in the return vector are malloc'd, and keep their trailing newlines (if they had them).
 */
size_t read_file_break_lines(Vector* ret, FILE* infile) {
    const size_t BLOCKSIZE = 4096;
    char read_buf[BLOCKSIZE+1];
    String* save = make_String("");
    read_buf[BLOCKSIZE] = 0;
    size_t total_copied = 0;
    while (true) {
        ssize_t num_read = fread(read_buf, sizeof(char), BLOCKSIZE, infile);
        // TODO: error check
        if (num_read == 0) {
            Vector_push(ret, String_to_cstr(save));
            return total_copied;
        }
        read_buf[num_read] = 0;
        char* scan_start = read_buf;
        size_t remaining_size = num_read;
        while (true) {
            char* split_loc = strchr(scan_start, '\n');
            if (split_loc == NULL) {
                if (remaining_size != strlen(scan_start)) {
                    int* x = 0;
                    *x = 1;
                }
                Strncats(&save, scan_start, remaining_size);
                break;
            }
            size_t char_idx = (split_loc - scan_start);
            if (save->length) {
                Strncats(&save, scan_start, char_idx+1);
                Vector_push(ret, String_to_cstr(save));

                save = make_String("");
            }
            else {
                Vector_push(ret, strndup(scan_start, char_idx+1));
            }
            scan_start = split_loc+1;
            remaining_size -= char_idx + 1;
        }
        total_copied += num_read;
    }
}

int Buffer_search_char(Buffer* buf, EditorContext* ctx, char c, bool direction) {
    ssize_t offset = 1;
    if (!direction) {
        offset = -1;
    }
    ssize_t current_pos = ctx->jump_col + offset;
    char* line = *(Buffer_get_line_abs(buf, ctx->jump_row));
    while (current_pos >= 0 && line[current_pos]) {
        if (line[current_pos] == c) {
            ctx->jump_col = current_pos;
            return 0;
        }
        current_pos += offset;
    }
    return -1;
}

/**
 * Alphnun_, punct, or whitespace.
 * if skip_punct, punct and word are treated the same.
 */
int __get_type(char c, bool skip_punct) {
    if (isspace(c)) { return 0; }
    if (isalnum(c) || c == '_') { return 1; }
    if (skip_punct) return 1;
    return 2;
}

int Buffer_skip_word(Buffer* buf, EditorContext* ctx, bool skip_punct) {
    size_t current_pos = ctx->jump_col;
    size_t current_row = ctx->jump_row;
    char* line = *(Buffer_get_line_abs(buf, current_row));

    char c = line[current_pos];
    if (c == '\0') { return -1; }   // NOTE: the only way this can fail is if you start at EOF.
                                    // Otherwise you can always move forward (even if the ending position is EOF).
    int search_type = __get_type(c, skip_punct);
    ++current_pos;
    while ((c = line[current_pos])) {
        int c_type = __get_type(c, skip_punct);
        if (c_type != search_type) {
            if (c_type == 0) {
                search_type = 0; // Other types "devolve" to space
            }
            else {
                ctx->jump_col = current_pos;
                ctx->jump_row = current_row;
                return 0;
            }
        }
        current_pos++;
        if (line[current_pos] == '\0' && current_row + 1 < Buffer_get_num_lines(buf)) {
            line = *(Buffer_get_line_abs(buf, current_row + 1));
            current_row++;
            current_pos = 0;
        }
    }
    ctx->jump_col = current_pos;
    ctx->jump_row = current_row;
    return 0;
}
