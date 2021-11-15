#pragma once
#include <stddef.h>

#include "../common.h"

typedef struct Macro {
    bool active;
    Vector /*Keystroke* */ keypresses;
} Macro;

Macro* current_recording_macro;

void Macro_push(Macro* this, char c, int control);

struct EditorAction {
    union {
        String* value;
    };
    size_t num_value;
    /**
     * Return 1: Keep me on the stack
     * Return 2: I absorbed this char and am complete; resolve now!
     * Return 3: ERROR! Cannot absorb char and would lead to weird behavior, cancel
     * Return zero: I can't absorb this char, try making a new action and linking
     */
    int (*update) (struct EditorAction* this, char, int);
    void (*resolve) (struct EditorAction* this, EditorContext*);
    struct EditorAction* child;

    /**
     * Override this to provide custom repeat logic (or optimized repeat logic).
     *
     * Return 0: Use default repeat (loop) (Or null)
     * Return 1: Custom repeat processed.
     */
    int (*repeat) (struct EditorAction* this, EditorContext*, size_t);
};

typedef struct EditorAction EditorAction;

void EditorAction_destroy(EditorAction*);

extern EditorAction* (*action_jump_table[]) (int);
extern ActionType action_type_table[];
Vector* /*EditorAction* */ action_stack;

void init_actions();

/**
 * Process a char (+ control) using the current action stack.
 * 
 */
int process_action(char, int);

void clear_action_stack();

/**
 * Return a C string representing the action stack (or null if no stack).
 * DO NOT FREE THIS!
 */
char* format_action_stack();

/**
 * Try to make an EditorAction corresponding to "numbers" (repeat).
 * Returns null on failure (bad input).
 */
EditorAction* make_NumberAction(char);
