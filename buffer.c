#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "buffer.h"
#include "utils.h"

int ED_INSERT = 0;
int ED_DELETE = 1;
int ED_FILE   = 2;

EditorAction* make_EditorAction(size_t start, size_t size, EditorActionType type) {
    //TODO error checking
    EditorAction* act = malloc(sizeof(EditorAction));
    inplace_make_EditorAction(act, start, size, type);
    return act;
}

void inplace_make_EditorAction(EditorAction* act, size_t start, size_t size,
                                    EditorActionType type) {
    act->start_idx = start;
    act->size = size;
    act->index_in_parent = 0;
    act->parent = NULL;
    inplace_make_Vector(&act->nested, 10);
    act->type = type;
}

void EditorAction_add_child(EditorAction* parent, EditorAction* child) {
    child->parent = parent;
    size_t insert_idx = 0;
    for(; insert_idx < parent->nested.size; ++insert_idx) {
        EditorAction* act = parent->nested.elements[insert_idx];
        if (act->start_idx >= child->start_idx) break;
    }
    child->index_in_parent = insert_idx;
    Vector_insert(&parent->nested, insert_idx, child);
    //TODO MERGING...
}

void EditorAction_destroy(EditorAction* this) {
    Vector_destroy(&this->nested);
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
        inplace_make_EditorAction(&buf->root, 0, 0, ED_INSERT);
        buf->root.buf = strdup("");
        buf->file = NULL;
        Vector_push(&buf->lines, buf->root.buf);
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

        inplace_make_EditorAction(&buf->root, 0, n_read, ED_FILE);
        buf->root.file = infile;
        buf->file = buf->root.file;
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
    buf->top_left_file_pos = 0;
    buf->last_pos = 0;
    buf->active_edit = NULL;
}
    
void Buffer_destroy(Buffer* buf) {
    EditorAction_destroy(&buf->root);
    Deque_destroy(&buf->undo_buffer);
    Vector_destroy(&buf->lines);
    fclose(buf->swapfile);
    if (buf->file != NULL) {
        fclose(buf->file);
    }
}

void resolve_index(EditorAction* parent, size_t index, EditorContext* ret) {
    int i = 0;
    Vector* actions = &parent->nested;
    for (; i < actions->size; ++i) {
        EditorAction* root = actions->elements[i];
        if (index < root->start_idx) {
            ret->index = index;
            ret->action = parent;
            return;
        }
        if (root->type == ED_INSERT) {
            if (index < root->start_idx + root->size) {
                // resolve_index(root, index - root->start_idx, ret);
                actions = &root->nested;
                index -= root->start_idx;
                parent = root;
                i = 0;
                continue;
            }
            index -= root->size;
        }
        else if (root->type == ED_DELETE) {
            index += root->size;
        }
    }
    ret->index = index;
    ret->action = parent;
}

int write_actions_string(char* out, EditorAction* parent, size_t* n_char) {
    if (parent->type != ED_INSERT) {
        return 1;
    }
    char* cur = out;
    size_t copy_start = 0;
    Vector* actions = &parent->nested;
    size_t n_copy;
    size_t total = 0;
    for (int i = 0; i < actions->size; ++i) {
        EditorAction* act = actions->elements[i];
        size_t action_start = act->start_idx;
        n_copy = action_start - copy_start;
        if (n_copy > 0) {
            memcpy(cur, parent->buf + copy_start, n_copy * sizeof(char));
            cur += n_copy;
            copy_start += n_copy;
            total += n_copy;
        }
        if (act->type == ED_DELETE) {
            copy_start += act->size;
        }
        else if (act->type == ED_INSERT) {
            write_actions_string(cur, act, &n_copy);
            cur += n_copy;
            total += n_copy;
        }
    }
    n_copy = parent->size - copy_start;
    if (n_copy > 0) {
        memcpy(cur, parent->buf + copy_start, n_copy * sizeof(char));
        total += n_copy;
    }
    *n_char = total;
    return 0;
}

int _write_actions(FILE* out, EditorAction* parent, size_t* n_char) {
    if (parent->type != ED_INSERT) {
        return 1;
    }
    Vector* actions = &parent->nested;
    size_t copy_start = 0;
    for (int i = 0; i < actions->size; ++i) {
        EditorAction* act = actions->elements[i];
        size_t action_start = act->start_idx;
        size_t n_copy = action_start - copy_start;
        if (n_copy > 0) {
            if (n_copy != fwriteall(parent->buf + copy_start, sizeof(char), n_copy, out)) {
                return -1;
            }
            copy_start += n_copy;
            *n_char += n_copy;
        }
        if (act->type == ED_DELETE) {
            copy_start += act->size;
        }
        else if (act->type == ED_INSERT) {
            _write_actions(out, act, n_char);
        }
    }
    size_t n_copy = parent->size - copy_start;
    if (n_copy > 0) {
        if (n_copy != fwriteall(parent->buf + copy_start, sizeof(char), n_copy, out)) {
            return -1;
        }
        *n_char += n_copy;
    }
    return 0;
}

int write_actions(FILE* out, EditorAction* parent, size_t* n_char) {
    if (parent->type == ED_INSERT) {
        *n_char = 0;
        return _write_actions(out, parent, n_char);
    }
    if (parent->type != ED_FILE) {
        return 1;
    }
    FILE* src = parent->file;
    int res = fseek(src, 0L, SEEK_SET);
    if (res) return res;

    Vector* actions = &parent->nested;
    *n_char = 0;
    size_t copy_start = 0;
    for (int i = 0; i < actions->size; ++i) {
        EditorAction* act = actions->elements[i];
        size_t action_start = act->start_idx;
        size_t n_copy = action_start - copy_start;
        if (n_copy > 0) {
            if (n_copy != fcopy(out, src, n_copy)) {
                return -1;
            }
            copy_start += n_copy;
            *n_char += n_copy;
        }
        if (act->type == ED_DELETE) {
            res = fseek(src, act->size, SEEK_CUR);
            if (res) return res;
            copy_start += act->size;
        }
        else if (act->type == ED_INSERT) {
            _write_actions(out, act, n_char);
        }
    }
    *n_char += fcopy(out, src, (size_t)-1);
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

