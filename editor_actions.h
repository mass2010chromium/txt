#pragma once
#include <stddef.h>

#include "common.h"

//typedef int ActionType;
//const ActionType AT_NONE   = 0;
//const ActionType AT_MOVE   = 1;
//const ActionType AT_DELETE = 2;

struct EditorAction {
    union {
        String* value;
        size_t num_value;
    };
    /**
     * Return 1: Keep me on the stack
     * Return 2: I absorbed this char and am complete; resolve now!
     * Return zero: I can't absorb this char, try making a new action and linking
     */
    int (*update) (struct EditorAction* this, char, int);
    void (*resolve) (struct EditorAction* this, EditorContext*);

    /**
     * Override this to provide custom repeat logic (or optimized repeat logic).
     */
    void (*repeat) (struct EditorAction* this, size_t);
    struct EditorAction* parent;
    struct EditorAction* child;
};

typedef struct EditorAction EditorAction;

/**
 * Try to make an EditorAction corresponding to "numbers" (repeat).
 * Returns null on failure (bad input).
 */
EditorAction* make_NumberAction(char);
