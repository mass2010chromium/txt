#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <sys/types.h>

#include "Deque.h"
#include "Vector.h"

/**
 * History, for undo or command history.
 * Can have fixed limit or infinite.
 */

typedef void (*destructor_t)(void*);

struct History {
    bool unlimited;
    Vector stack;   // Used for both if not unlimited.
    Deque deque;    // Only used if not unlimited.
    size_t cursor;  // Only used if unlimited. Points to past-end. Maybe unionify?
    destructor_t destructor_handle; // Destructor function needed for push...
};

typedef struct History History;

/**
 * Make a new history object, with the specified capcity.
 */
History* make_History(size_t size, destructor_t destructor);
void inplace_make_History(History* history, size_t, destructor_t);

/**
 * Push a new item into the history.
 * NOTE: The History object has ownership of all its items.
 */
void History_push(History*, void*);

/**
 * Scroll the history buffer.
 * Positive means back in time.
 * Returns a pointer to the new "now".
 */
void** History_scroll(History*, ssize_t amount);

/**
 * Equivalent to History.scroll(0).
 * Get the item at the end of the deque.
 * Returns a pointer to "now".
 */
void** History_peek(History*);

/**
 * Get the current depth.
 * Equal to the amount you can scroll back.
 * Depth is at most (size), and at least 0.
 * 0 means empty history.
 */
size_t History_get_depth(History*);

/**
 * NOTE: The History object has ownership of all its items.
 * They will be destroyed here!
 */
void History_destroy(History*);
