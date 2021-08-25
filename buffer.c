#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "buffer.h"
#include "utils.h"

EditorAction* make_EditorAction(size_t start, char* old_content) {
    EditorAction* ret = malloc(sizeof(EditorAction));
    inplace_make_EditorAction(ret, start, old_content);
    return ret;
}

void inplace_make_EditorAction(EditorAction* ed, size_t start, char* old_content) {
    ed->undo_index = -1;
    ed->start_idx = start;
    ed->old_content = strdup(old_content);
    ed->new_content = strdup(old_content);
}

void EditorAction_destroy(EditorAction* ed) {
    free(ed->old_content);
    free(ed->new_content);
}

Buffer* make_Buffer(char* filename) {
    Buffer* ret = malloc(sizeof(Buffer));
    inplace_make_Buffer(ret, filename);
    return ret;
}

void inplace_make_Buffer(Buffer* buf, char* filename) {
    inplace_make_Deque(&buf->undo_buffer, 100);
    inplace_make_Vector(&buf->lines, 100);
    if (filename == NULL) {
        filename = "__tmp__";
        buf->file = NULL;
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

        buf->file = infile;
    }
    char* end = ".swp";
    char* dest = malloc(strlen(filename) + strlen(end) + 1);
    dest[0] = '\0';
    strcat(dest, filename);
    strcat(dest, end);
    buf->swapfile = fopen(dest, "w+");
    free(dest);
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
    fclose(buf->swapfile);
    if (buf->file != NULL) {
        fclose(buf->file);
    }
}

char** Buffer_get_line(Buffer* buf, size_t y) {
    if (y + buf->top_row >= buf->lines.size) return NULL;
    return (char**) &(buf->lines.elements[y + buf->top_row]);
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

