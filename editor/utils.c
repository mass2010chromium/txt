#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "utils.h"
#include "../common.h"

size_t fwriteall(void* buf, size_t size, size_t n_element, FILE* f) {
    // TODO technically dangerous but idrc
    size_t total = size * n_element;
    size_t total_written = 0;
    while (total_written != total) {
        size_t n_written = fwrite(buf, 1, total, f);
        if (n_written == 0) {
            return total_written;
        }
        total_written += n_written;
        buf += n_written;
        total -= n_written;
    }
    return total_written;
}

size_t fcopy(FILE* dest, FILE* src, size_t bytes) {
    size_t total_copied = 0;
    const size_t BLOCKSIZE = 4096;
    char buf[BLOCKSIZE];
    while (total_copied != bytes) {
        size_t copy_block = bytes;
        if (copy_block > BLOCKSIZE) copy_block = BLOCKSIZE;
        size_t num_read = fread(buf, sizeof(char), copy_block, src);
        if (num_read == 0) {
            return total_copied;
        }
        fwriteall(buf, sizeof(char), num_read, dest);
        total_copied += num_read;
    }
    return total_copied;
}

/**
 * "Normalize" a set of row, col pairs such that
 * (ret->start_row, ret->start_col) comes "before"
 * (ret->jump_row, ret->jump_col).
 *
 * If c1 or c2 is negative, both jump columns are set to -1.
 */
void normalize_context(EditorContext* ret, ssize_t r1, ssize_t c1,
                                           ssize_t r2, ssize_t c2) {
    if (r1 == r2) {
        ret->start_row = r1;
        ret->jump_row = r1;
        ret->start_col = min_d(c1, c2);
        ret->jump_col = max_d(c1, c2);
    }
    else if (r1 > r2) {
        ret->start_row = r2;
        ret->start_col = c2;
        ret->jump_row = r1;
        ret->jump_col = c1;
    }
    else {
        ret->start_row = r1;
        ret->start_col = c1;
        ret->jump_row = r2;
        ret->jump_col = c2;
    }
    if (ret->start_col == -1 || ret->jump_col == -1) {
        ret->start_col = -1;
        ret->jump_col = -1;
    }
    print("pre: %ld %ld %ld %ld\n", r1, c1, r2, c2);
    print("normalized: %ld %ld %ld %ld\n", ret->start_row, ret->start_col, ret->jump_row, ret->jump_col);
}

void EditorContext_normalize(EditorContext* self) {
    normalize_context(self, self->start_row, self->start_col,
                            self->jump_row, self->jump_col);
}

bool is_whitespace(char c) {
    return (c == ' ' || c == '\t');
}
