#pragma once
#include <stdbool.h>
#include <stdio.h>

#include "Deque.h"
#include "Vector.h"
#include "String.h"

#include "gap_buffer.h"

/**
 * Attempt to write the contents of `buf` to the passed file `f`.
 * Will retry if the write fails or is otherwise incomplete.
 */
size_t fwriteall(void* buf, size_t size, size_t n_element, FILE* f);

/**
 * Reads the content of `src` into a buffer, then writes the contents
 * to `dest` using fwriteall.s
 */
size_t fcopy(FILE* dest, FILE* src, size_t bytes);

#define __MINMAX_GEN(name, type) \
static inline type max_ ## name (type a, type b) {\
    if (a > b) return a; \
    return b; \
} \
static inline type min_ ## name (type a, type b) {\
    if (a > b) return b; \
    return a; \
}

__MINMAX_GEN(u, size_t)

__MINMAX_GEN(d, ssize_t)

/**
 * Checks if the passed character is whitespace.
 * (Compares to ' ' and '\t').
 */
bool is_whitespace(char);
