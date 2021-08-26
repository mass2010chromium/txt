#include "editor_actions.h"

#include <stdlib.h>

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
}

EditorAction* make_NumberAction(char input) {
    int val = input - '0';
    if (val >= 0 && val < 10) {
        EditorAction* ret = malloc(sizeof(EditorAction));
        ret->update = &NumberAction_update;
        ret->resolve = &NumberAction_resolve;
        ret->parent = NULL;
        ret->child = NULL;
        ret->num_value = val;
        return ret;
    }
    return NULL;
}
