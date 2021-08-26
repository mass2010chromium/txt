#pragma once
#include <stddef.h>

#include "common.h"

typedef int ActionType;
extern const ActionType AT_NONE;
extern const ActionType AT_MOVE;
extern const ActionType AT_DELETE;

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
};

typedef struct EditorAction EditorAction;

EditorAction* (*action_jump_table[]) (void);
Vector /*EditorAction* */ action_stack;

void init_actions();

/**
 * Process a char (+ control) using the current action stack.
 * 
 */
int process_action(char, int);

/**
 * Try to make an EditorAction corresponding to "numbers" (repeat).
 * Returns null on failure (bad input).
 */
EditorAction* make_NumberAction(char);

EditorAction* make_j_Action();
