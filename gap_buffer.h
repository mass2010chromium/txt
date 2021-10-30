#pragma once

#include <stdio.h>

#include "utils.h"
#include "common.h"

#define DEFAULT_GAP_SIZE 1000

struct GapBuffer {
    char* content; //Do we need to keep track of stuff left of the buffer separately from stuff to the right? idk
    size_t total_size;
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
void gapBuffer_destroy(GapBuffer* buf);

/** Returns a string representation of the non-gap content in the buffer, allocated on the heap. */
char* gapBuffer_get_content(GapBuffer* buf);

/**
 * Attempts to insert (copy) the provided string content into the buffer.
 * If inserting this content would use up the entire gap, the gap is resized
 * to accomodate the content + DEFAULT_GAP_SIZE (1000).
 */
void gapBuffer_insert(GapBuffer* buf, const char* new_content);

/**
 * Attempts to delete `delete_size` bytes from the buffer, extending the gap
 * to the left by that many bytes. If delete_size would cause the gap to extend
 * past the start of the content buffer, it will instead only delete to the start
 * of the buffer.
 */
void gapBuffer_delete(GapBuffer* buf, size_t delete_size);

/**
 * Resizes the gap inside the buffer to accomodate target_size +
 * DEFAULT_GAP_SIZE (1000). Any buffer content past the previous gap
 * location is copied to a temporary array while the gap is extended
 * then moved back to the content buffer afterwards.
 */
void gapBuffer_resize(GapBuffer* buf, size_t target_size);

/**
 * Moves the gap in the buffer by the specified offset.
 * Positive offset moves forwards (right), negative offset moves backwards (left).
 */
void gapBuffer_move_gap(GapBuffer* buf, ssize_t offset);