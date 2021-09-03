#pragma once
#include <stddef.h>

#include "common.h"

struct EditorAction {
    union {
        String* value;
        size_t* num_value;
    };
    /**
     * Return 1: Keep me on the stack
     * Return 2: I absorbed this char and am complete; resolve now!
     * Return zero: I can't absorb this char, try making a new action and linking
     */
    int (*update) (struct EditorAction* this, char, int);
    void (*resolve) (struct EditorAction* this, EditorContext*);
    struct EditorAction* child;

    /**
     * Override this to provide custom repeat logic (or optimized repeat logic).
     */
    void (*repeat) (struct EditorAction* this, size_t);
};

typedef struct EditorAction EditorAction;

void EditorAction_destroy(EditorAction*);

extern EditorAction* (*action_jump_table[]) (void);
extern ActionType action_type_table[];
Vector /*EditorAction* */ action_stack;

void init_actions();

/**
 * Process a char (+ control) using the current action stack.
 * 
 */
int process_action(char, int);

void clear_action_stack();

/**
 * Try to make an EditorAction corresponding to "numbers" (repeat).
 * Returns null on failure (bad input).
 */
EditorAction* make_NumberAction(char);

EditorAction* make_h_action();
EditorAction* make_j_action();
EditorAction* make_k_action();
EditorAction* make_l_action();
EditorAction* make_i_action();
EditorAction* make_o_action();

EditorAction* make_u_action();
EditorAction* make_p_action();

EditorAction* make_x_action();
EditorAction* make_d_action();

EditorAction* make_A_action();

EditorAction* make_DOLLAR_action();
