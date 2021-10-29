#pragma once

#include <stdio.h>

#include "utils.h"
#include "common.h"

struct GapBuffer {
    char* content; //Do we need to keep track of stuff left of the buffer separately from stuff to the right? idk
    size_t gap_size;
    size_t gap_start; /* Current position of the gap start (cursor). Defaults to 0. */
    size_t gap_end; /* Current position of the gap end. Defaults to 0 + gap_size. */
}
typedef struct GapBuffer GapBuffer;

/**
 * Creates a gap buffer for the line at line_num in the lines vector contained
 * by the given Buffer.
 */
GapBuffer* make_GapBuffer(const char* content, ssize_t line_num);
void inplace_make_GapBuffer(GapBuffer* buf, const char* content, size_t gap_size);

void gapBuffer_insert(GapBuffer* buf, const char* new_content);
void gapBuffer_resize(GapBuffer* buf, size_t target_size);