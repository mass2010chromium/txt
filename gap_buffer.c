#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "buffer.h"
#include "gap_buffer.h"
#include "utils.h"

Buffer* make_GapBuffer(const char* content, size_t gap_size) {
    if (buf == NULL) {
        return NULL;
    }
    GapBuffer* ret = malloc(sizeof(GapBuffer));
    char** content = Buffer_get_line(buf, line_num);
    inplace_make_GapBuffer(ret, content, gap_size);
    return ret;
}

void inplace_make_GapBuffer(GapBuffer* buf, const char* content, size_t gap_size) {
    buf->gap_size = gap_size;
    buf->gap_start = 0;
    buf->gap_end = gap_size;
    size_t content_size = 0;
    if (content != NULL) {
        content_size = strlen(*content);
    }
    buf->content = calloc(content_size, sizeof(char));
    if (content != NULL) {
        memcpy(buf->content + gap_size, *content);
    }
    buf->total_size = gap_size + strlen(content)
}

void gapBuffer_insert(GapBuffer* buf, const char* new_content) {
    size_t insert_size = strlen(new_content);
    if (buf->gap_size >= insert_size)) {
        memcpy(buf->gap_start, new_content);
        buf->gap_size -= insert_size;
        buf->gap_start += insert_size;
    } else {
        buffer_resize(buf, insert_size);
        memcpy(buf->gap_start, new_content);
        buf->gap_size -= insert_size;
        buf->gap_start += insert_size;
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

void gapBuffer_resize(GapBuffer* buf, size_t target_size) {
    //Create temp array to store contents of buffer after gap
    char* temp_buffer = malloc(buf->total_size - buf->gap_end);
    size_t copy_length = buf->total_size - buf->gap_end;
    memmove(temp_buffer, buf->content + gap_end, copy_length);
    //Calloc gap size to accomodate target_size (plus some other growth factor?)
    buf->content = realloc(buf->content, buf->total_size + target_size + DEFAULT_GAP_SIZE);
    buf->total_size += target_size + DEFAULT_GAP_SIZE;
    buf->gap_size += DEFAULT_GAP_SIZE;
    buf->gap_end += DEFAULT_GAP_SIZE;
    //Move contents of temp array back to gap buffer, after the new gap end
    memmove(buf->content + buf->gap_end, temp_buffer, copy_length);
}

void gapBuffer_move_gap(GapBuffer* buf, ssize_t offset) {
    if (offset == 0 || buf->gap_size == buf->total_size) {
        //Don't need to move anything
        return;
    }
    if (buf->gap_start + offset < 0 || buf->gap_end + offset >= buf->total_size) {
        //Out of bounds
        return;
    }
    size_t new_start = buf->gap_start + offset;
    size_t new_end = buf->gap_end + offset;
    //Move memory that gap will occupy into space previously occupied by buffer
    memmove(buf->content + buf->gap_start, buf->content + new_start, buf->gap_size);
    memset(buf->content + new_start, 0, buf->gap_size)
}

char* gapBuffer_get_content(GapBuffer* buf) {
    char* ret = malloc(buf->total_size - buf->gap_size);
    memcpy(ret, buf->content, buf->gap_start);
    memcpy(ret + buf->gap_start, buf->content + buf->gap_end, buf->total_size - buf->gap_end);
    return ret;
}

void gapBuffer_destroy(GapBuffer* buf) {
    free(buf->content);
}