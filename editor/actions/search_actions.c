/**
 * Editor actions that involve searching for things.
 * Contents:
 *  f       search forwards for char
 *  F       search backwards for char
 *  w       move word, break punct and space
 *  W       move word, break punct
 *  /       search for regex forward across lines
 *  ?       search for regex backward across lines
 *  n       repeat previous word search
 *  N       reverse previous word search
 */
 
int f_action_update(EditorAction* this, char input, int control) {
    String_push(&this->value, input);
    return 2;
}

void f_action_resolve(EditorAction* this, EditorContext* ctx) {
    if (Strlen(this->value) == 2) {
        int res = Buffer_search_char(ctx->buffer, ctx, this->value->data[1], true);
        if (res == -1) { ctx->action = AT_ESCAPE; }
        else { ctx->action = AT_MOVE; }
    }
}

EditorAction* make_f_action(int control) {
    EditorAction* ret = make_DefaultAction("f");
    ret->update = &f_action_update;
    ret->resolve = &f_action_resolve;
    return ret;
}

int F_action_update(EditorAction* this, char input, int control) {
    String_push(&this->value, input);
    return 2;
}

void F_action_resolve(EditorAction* this, EditorContext* ctx) {
    if (Strlen(this->value) == 2) {
        int res = Buffer_search_char(ctx->buffer, ctx, this->value->data[1], false);
        if (res == -1) { ctx->action = AT_ESCAPE; }
        else { ctx->action = AT_MOVE; }
    }
}

EditorAction* make_F_action(int control) {
    EditorAction* ret = make_DefaultAction("F");
    ret->update = &F_action_update;
    ret->resolve = &F_action_resolve;
    return ret;
}

int slash_action_update(EditorAction* this, char input, int control) {
    //TO-DO: Catch special cases? Ask Jing about it
    if (input == BYTE_ESC) {
        return 0;
    } else if (input == BYTE_BACKSPACE) {
        String_pop(this->value);
        return Strlen(this->value) > 0;
    } else if (input != '\n') {
        String_push(&this->value, input);
        return 1;
    } else {
        return 2;
    }
}

bool prev_search_order;
String* prev_search_str = NULL;

void slash_action_resolve(EditorAction* this, EditorContext* ctx) {
    if (this->child != NULL) {
        (*this->child->resolve)(this->child, ctx);
        return;
    }
    if (Strlen(this->value) > 1) {
        char* str = this->value->data + 1;
        prev_search_order = true;
        if (prev_search_str == NULL) { prev_search_str = make_String(this->value->data + 1); }
        else { Strcpys(&prev_search_str, this->value->data + 1); }
        Buffer_find_str(ctx->buffer, ctx, str, true, true);
        if (ctx->action == AT_DELETE && ctx->jump_col > 0) {
            --ctx->jump_col;
        }
        ctx->action = AT_MOVE;
    }
}

EditorAction* make_slash_action(int control) {
    EditorAction* ret = make_DefaultAction("/");
    ret->update = &slash_action_update;
    ret->resolve = &slash_action_resolve;
    return ret;
}

int question_action_update(EditorAction* this, char input, int control) {
    if (input == BYTE_ESC) {
        return 0;
    } else if (input == BYTE_BACKSPACE) {
        String_pop(this->value);
        return Strlen(this->value) > 0;
    } else if (input != '\n') {
        String_push(&this->value, input);
        return 1;
    } else {
        return 2;
    }
}

void question_action_resolve(EditorAction* this, EditorContext* ctx) {
    if (this->child != NULL) {
        (*this->child->resolve)(this->child, ctx);
        return;
    }
    ctx->action = AT_MOVE;
    if (Strlen(this->value) > 1) {
        char* str = this->value->data + 1;
        prev_search_order = false;
        if (prev_search_str == NULL) { prev_search_str = make_String(this->value->data + 1); }
        else { Strcpys(&prev_search_str, this->value->data + 1); }
        Buffer_find_str(ctx->buffer, ctx, str, true, false);
    }
}

EditorAction* make_question_action(int control) {
    EditorAction* ret = make_DefaultAction("?");
    ret->update = &question_action_update;
    ret->resolve = &question_action_resolve;
    return ret;
}

void n_action_resolve(EditorAction* this, EditorContext* ctx) {
    ctx->action = AT_MOVE;
    if (prev_search_str != NULL) {
        Buffer_find_str(ctx->buffer, ctx, prev_search_str->data, true, prev_search_order);
    }
}

EditorAction* make_n_action(int control) {
    EditorAction* ret = make_DefaultAction("n");
    ret->resolve = &n_action_resolve;
    return ret;
}

void N_action_resolve(EditorAction* this, EditorContext* ctx) {
    ctx->action = AT_MOVE;
    if (prev_search_str != NULL) {
        Buffer_find_str(ctx->buffer, ctx, prev_search_str->data, true, !prev_search_order);
    }
}

EditorAction* make_N_action(int control) {
    EditorAction* ret = make_DefaultAction("N");
    ret->resolve = &N_action_resolve;
    return ret;
}
