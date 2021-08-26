#include "editor_actions.h"

#include <stdlib.h>

const ActionType AT_NONE   = 0;
const ActionType AT_MOVE   = 1;
const ActionType AT_DELETE = 2;

EditorAction* (*action_jump_table[256]) (void) = {0};
Vector /*EditorAction* */ action_stack = {0};

void init_actions() {
    action_jump_table['j'] = &make_j_Action;
    inplace_make_Vector(&action_stack);
}

/**
 * Process a char (+ control) using the current action stack.
 * 
 */
int process_action(char c, int control) {
    if (action_stack.size != 0) {
        EditorAction* top = action_stack.elements[action_stack.size - 1];
        int res = (*top->update)(top, c, control);
        if (res == 2) {
            // resolve!
        }
        else if (res == 1) {
            return;
        }
    }
    EditorAction* new_action = make_NumberAction(c);
    if (new_action != NULL) {
        Vector_push(&action_stack, new_action);
        if (action_stack.size == 1) {
            return;
        }
        EditorAction* prev = action_stack.elements[action_stack.size - 2];
        return;
    }
}

int NumberAction_update(EditorAction* this, char input, int control) {
    int val = input - '0';
    if (val >= 0 && val < 10) {
        this->num_value = this->num_value * 10 + val;
        return 1;
    }
    return 0;
}

void NumberAction_resolve(EditorAction* this, EditorContext* ctx) {
    if (this->child == NULL) {
        return;
    }
    for (int i = 0; i < this->num_value; ++i) {
        (*this->child->resolve)(this->child, ctx);
    }
}

EditorAction* make_NumberAction(char input) {
    int val = input - '0';
    if (val >= 0 && val < 10) {
        EditorAction* ret = malloc(sizeof(EditorAction));
        ret->update = &NumberAction_update;
        ret->resolve = &NumberAction_resolve;
        ret->num_value = val;
        return ret;
    }
    return NULL;
}

void j_Action_resolve(EditorAction* this, EditorContext* ctx) {
    ctx->action = AT_MOVE;
    ctx->jump_row -= 1;
}

EditorAction* make_j_Action() {
    EditorAction* ret = malloc(sizeof(EditorAction));
    ret->update = NULL;
    ret->resolve = &j_Action_resolve;
    ret->value = make_String("j");
    return ret;
}
