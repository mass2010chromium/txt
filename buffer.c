#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/sendfile.h>

#include "buffer.h"
#include "utils.h"

EditorAction* make_EditorAction(size_t undo, size_t start, char* old_content) {
    EditorAction* ret = malloc(sizeof(EditorAction));
    inplace_make_EditorAction(ret, undo, start, old_content);
    return ret;
}

void inplace_make_EditorAction(EditorAction* ed, size_t undo, size_t start, char* old_content) {
    ed->undo_index = undo;
    ed->start_idx = start;
    ed->old_content = strdup(old_content);
    ed->new_content = strdup(old_content);
}

void EditorAction_destroy(EditorAction* ed) {
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
        }
        else {
            n_read = 0;
        }
    }
    buf->swapfile = NULL;
    buf->name = filename;
    buf->cursor_row = 1;
    buf->cursor_col = 1;
    buf->natural_col = 1;
    buf->top_row = 0;
    buf->top_left_file_pos = 0;
    buf->last_pos = 0;
}
    
void Buffer_destroy(Buffer* buf) {
    Deque_destroy(&buf->undo_buffer);
    Vector_destroy(&buf->lines);
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

