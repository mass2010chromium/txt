#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "utils.h"

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

bool is_whitespace(char c) {
    return (c == ' ' || c == '\t');
}
