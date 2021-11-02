#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "buffer.h"
#include "gap_buffer.h"
#include "utils.h"

GapBuffer* make_GapBuffer(const char* content, size_t gap_size) {
    GapBuffer* ret = malloc(sizeof(GapBuffer));
    inplace_make_GapBuffer(ret, content, gap_size);
    return ret;
}

void inplace_make_GapBuffer(GapBuffer* buf, const char* content, size_t gap_size) {
    buf->gap_size = gap_size;
    buf->gap_start = 0;
    buf->gap_end = gap_size;
    size_t content_size = 0;
    if (content != NULL) {
        content_size = strlen(content);
    }
    buf->content = calloc(content_size + gap_size + 1, sizeof(char));
    if (content != NULL) {
        memcpy(buf->content + gap_size, content, content_size);
    }
    buf->total_size = gap_size + content_size;
}

void gapBuffer_insert(GapBuffer* buf, const char* new_content) {
    size_t insert_size = strlen(new_content);
    gapBuffer_insertN(buf, new_content, insert_size);
}

void gapBuffer_insertN(GapBuffer* buf, const void* data, size_t n) {
    if (buf->gap_size >= n) {
        memcpy(buf->content + buf->gap_start, data, n);
        buf->gap_size -= n;
        buf->gap_start += n;
    } else {
        gapBuffer_resize(buf, n);
        memcpy(buf->content + buf->gap_start, data, n);
        buf->gap_size -= n;
        buf->gap_start += n;
    }
}

void gapBuffer_delete(GapBuffer* buf, size_t delete_size) {
    if (delete_size > buf->gap_start) {
        delete_size = buf->gap_start;
    }
    buf->gap_size += delete_size;
    buf->gap_start -= delete_size;
    memset(buf->content + buf->gap_start, 0, delete_size);
}

void gapBuffer_delete_right(GapBuffer* buf, size_t delete_size) {
    if (delete_size > buf->total_size - buf->gap_end) {
        delete_size = buf->total_size - buf->gap_end;
    }
    buf->gap_size += delete_size;
    memset(buf->content + buf->gap_end, 0, delete_size);
    buf->gap_end += delete_size;
    
}

void gapBuffer_resize(GapBuffer* buf, size_t target_size) {
    if (target_size == 0) {
        return;
    }
    //Create temp array to store contents of buffer after gap
    char* temp_buffer = malloc(buf->total_size - buf->gap_end);
    size_t copy_length = buf->total_size - buf->gap_end;
    memmove(temp_buffer, buf->content + buf->gap_end, copy_length);
    //Calloc gap size to accomodate target_size (plus some other growth factor?)
    int growth_k = (int)floor((DEFAULT_GAP_SIZE + target_size) / (double)DEFAULT_GAP_SIZE);
    size_t growth_amt = growth_k * DEFAULT_GAP_SIZE;
    //size_t growth_amt = DEFAULT_GAP_SIZE;
    buf->content = realloc(buf->content, buf->total_size + growth_amt + 1);
    buf->total_size += growth_amt;
    buf->content[buf->total_size] = 0;
    buf->gap_size += growth_amt;
    buf->gap_end += growth_amt;
    //Move contents of temp array back to gap buffer, after the new gap end
    memmove(buf->content + buf->gap_end, temp_buffer, copy_length);
    free(temp_buffer);
}

void gapBuffer_move_gap(GapBuffer* buf, ssize_t offset) {
    if (offset == 0 || buf->gap_size == buf->total_size) {
        //Don't need to move anything
        return;
    }
    if (buf->gap_start + offset < 0 || buf->gap_end + offset > buf->total_size) {
        //Out of bounds, raise segfault to prevent weirdness from happening later
        *((int*)0) = 0;
        return;
    }
    size_t new_start = buf->gap_start + offset;
    size_t new_end = buf->gap_end + offset;
    if (offset > 0) {
        //copy overlap characters to left side of gap
        memmove(buf->content + buf->gap_start, buf->content + buf->gap_end, offset);
    } else {
        //copy overlap characters to right side of gap
        memmove(buf->content + new_end, buf->content + new_start, -offset);
    }
    //Clear new gap location
    memset(buf->content + new_start, 0, buf->gap_size);
    buf->gap_start = new_start;
    buf->gap_end = new_end;
}

char* gapBuffer_get_content(GapBuffer* buf) {
    //Allocate 1 extra byte so we can add a null-terminator at the end
    char* ret = malloc(buf->total_size - buf->gap_size + 1);
    memcpy(ret, buf->content, buf->gap_start);
    memcpy(ret + buf->gap_start, buf->content + buf->gap_end, buf->total_size - buf->gap_end);
    memset(ret + (buf->total_size - buf->gap_size), 0, 1);
    return ret;
}

void gapBuffer_destroy(GapBuffer* buf) {
    free(buf->content);
    buf->total_size = 0;
    buf->gap_end = 0;
    buf->gap_size = 0;
    buf->gap_start = 0;
    buf->content = NULL;
}

size_t gapBuffer_content_length(GapBuffer* buf) {
    return buf->total_size - buf->gap_size;
}
