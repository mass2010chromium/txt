#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/sendfile.h>

#include "buffer.h"
#include "utils.h"

Edit* make_Insert(size_t undo, size_t start_row, size_t start_col, char* new_content) {
    Edit* ret = malloc(sizeof(Edit));
    ret->undo_index = undo;
    ret->start_row = start_row;
    ret->start_col = start_col;
    ret->old_content = NULL;
    ret->new_content = make_String(new_content);
    return ret;
}

Edit* make_Delete(size_t undo, size_t start_row, size_t start_col, char* old_content) {
    Edit* ret = malloc(sizeof(Edit));
    ret->undo_index = undo;
    ret->start_row = start_row;
    ret->start_col = start_col;
    ret->old_content = strdup(old_content);
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
    inplace_make_Deque(&buf->undo_buffer, 100);
    inplace_make_Vector(&buf->redo_buffer, 100);
    inplace_make_Vector(&buf->lines, 100);
    buf->file = NULL;
    if (filename == NULL) {
        filename = "__tmp__";
        Vector_push(&buf->lines, strdup(""));
    }
    else {
        FILE* infile = fopen(filename, "r+");
        size_t n_read;
        if (infile != NULL)  {
            //TODO buffer/read not the whole file
            n_read = read_file_break_lines(&buf->lines, infile);
            if (n_read == 0) {
                Vector_push(&buf->lines, strdup(""));
            }
        }
        else {
            n_read = 0;
        }
    }
    buf->swapfile = NULL;
    buf->name = strdup(filename);
    buf->cursor_row = 0;
    buf->cursor_col = 0;
    buf->natural_col = 0;
    buf->top_row = 0;
    buf->top_left_file_pos = 0;
    buf->last_pos = 0;
}
    
void Buffer_destroy(Buffer* buf) {
    Deque_destroy(&buf->undo_buffer);
    Vector_destroy(&buf->redo_buffer);
    Vector_destroy(&buf->lines);
    free(buf->name);
    Buffer_close_files(buf);
}

void Buffer_open_files(Buffer* buf, const char* mode_infile, const char* mode_swapfile) {
    if (mode_infile != NULL) { buf->file = fopen(buf->name, mode_infile); }
    if (mode_swapfile != NULL) {
        char* end = ".swp";
        char* dest = malloc(strlen(buf->name) + strlen(end) + 1);
        dest[0] = '\0';
        strcat(dest, buf->name);
        strcat(dest, end);
        buf->swapfile = fopen(dest, mode_swapfile);
        free(dest);
    }
}


/**
 * Scroll by up to `amount` (signed). Positive is down.
 * Return: The actual amount scrolled
 */
ssize_t Buffer_scroll(Buffer* buf, size_t window_height, ssize_t amount) {
    ssize_t scroll_amount;
    if (-amount > buf->top_row) {
        scroll_amount = -buf->top_row;
        buf->top_row = 0;
        return scroll_amount;
    }
    else {
        size_t save = buf->top_row;
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

size_t Buffer_get_line_index(Buffer* buf, size_t y) {
    return y + buf->top_row;
}

char** Buffer_get_line(Buffer* buf, size_t y) {
    if (y + buf->top_row >= buf->lines.size) return NULL;
    return (char**) &(buf->lines.elements[y + buf->top_row]);
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
    return 0;
}

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
    Vector_clear(&buf->redo_buffer, 100);
}

void Buffer_undo_Edit(Buffer* buf, Edit* ed) {
    size_t index = ed->start_row;
    if (ed->old_content == NULL) {
        // Insert action. Undo by deleting.
        char* lineptr = buf->lines.elements[index];
        Vector_delete(&buf->lines, index);
        free(lineptr);
        return;
    }
    if (ed->new_content == NULL) {
        // Delete action. Undo by inserting.
        Vector_insert(&buf->lines, index, strdup(ed->old_content));
        return;
    }
    char** lineptr = (char**) &(buf->lines.elements[index]);
    free(*lineptr);
    *lineptr = strdup(ed->old_content);
}

int Buffer_undo(Buffer* buf, size_t undo_index) {
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
        Buffer_undo_Edit(buf, ed);
        Deque_pop_r(&buf->undo_buffer);
        Vector_push(&buf->redo_buffer, ed);
    }
}

int Buffer_redo(Buffer*, size_t undo_index);

/**
 * Read a file into a vector. One entry in the vector for each line in the file.
 * All strings in the return vector are malloc'd, and keep their trailing newlines (if they had them).
 */
size_t read_file_break_lines(Vector* ret, FILE* infile) {
    const size_t BLOCKSIZE = 4096;
    char read_buf[BLOCKSIZE+1];
    size_t save_size = 0;
    char* save = malloc(sizeof(char));
    *save = 0;
    read_buf[BLOCKSIZE] = 0;
    size_t total_copied = 0;
    while (true) {
        size_t num_read = fread(read_buf, sizeof(char), BLOCKSIZE, infile);
        if (num_read == 0) {
            if (*save) {
                Vector_push(ret, save);
            }
            else {
                free(save);
            }
            return total_copied;
        }
        read_buf[num_read] = 0;
        char* scan_start = read_buf;
        size_t remaining_size = num_read;
        while (true) {
            char* split_loc = strchr(scan_start, '\n');
            if (split_loc == NULL) {
                save = realloc(save, save_size + remaining_size + 1);
                memcpy(save+save_size, scan_start, remaining_size);
                save_size += remaining_size;
                save[save_size] = 0;
                break;
            }
            size_t char_idx = (split_loc - scan_start);
            if (*save) {
                save = realloc(save, save_size + remaining_size + 1);
                memcpy(save+save_size, scan_start, remaining_size);
                save_size += remaining_size;
                save[save_size] = 0;
                Vector_push(ret, save);

                save = malloc(sizeof(char));
                *save = 0;
                save_size = 0;
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
