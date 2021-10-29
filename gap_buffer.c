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
}

void buffer_insert(GapBuffer* buf, const char* new_content) {
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

void buffer_resize(GapBuffer* buf, size_t target_size) {
    //Create temp array to store contents of buffer after gap
    //Calloc gap size to accomodate target_size (plus some other growth factor?)
    //Move contents of temp array back to gap buffer, after the new gap end
}

// -----CONTENT
// C-----ONTENT
// CRAB--ONTENT