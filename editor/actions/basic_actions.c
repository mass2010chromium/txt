/**
 * Most basic editor actions.
 * Contents:
 *  h       move left
 *  j       move down
 *  k       move up
 *  l       move right
 *  i       insert mode
 *  ESC     exit insert mode, cancel
 */

void h_action_resolve(EditorAction* this, EditorContext* ctx) {
    ctx->action = AT_MOVE;
    ctx->jump_col -= 1;
}

EditorAction* make_h_action(int control) {
    EditorAction* ret = make_DefaultAction("h");
    ret->resolve = &h_action_resolve;
    return ret;
}

void j_action_resolve(EditorAction* this, EditorContext* ctx) {
    ctx->action = AT_MOVE;
    ctx->jump_row += 1;
    ctx->jump_col = ctx->buffer->natural_col;
}

EditorAction* make_j_action(int control) {
    EditorAction* ret = make_DefaultAction("j");
    ret->resolve = &j_action_resolve;
    return ret;
}

void k_action_resolve(EditorAction* this, EditorContext* ctx) {
    ctx->action = AT_MOVE;
    ctx->jump_row -= 1;
    ctx->jump_col = ctx->buffer->natural_col;
}

EditorAction* make_k_action(int control) {
    EditorAction* ret = make_DefaultAction("k");
    ret->resolve = &k_action_resolve;
    return ret;
}

void l_action_resolve(EditorAction* this, EditorContext* ctx) {
    ctx->action = AT_MOVE;
    ctx->jump_col += 1;
}

EditorAction* make_l_action(int control) {
    EditorAction* ret = make_DefaultAction("l");
    ret->resolve = &l_action_resolve;
    return ret;
}

void i_action_resolve(EditorAction* this, EditorContext* ctx) {
    ctx->action = AT_OVERRIDE;
    editor_new_action();
    current_mode = EM_INSERT;
    begin_insert();
    editor_align_tab();
}

EditorAction* make_i_action(int control) {
    EditorAction* ret = make_DefaultAction("i");
    ret->resolve = &i_action_resolve;
    return ret;
}

void ESC_action_resolve(EditorAction* this, EditorContext* ctx) {
    ctx->action = AT_ESCAPE;
}

EditorAction* make_ESC_action(int control) {
    EditorAction* ret;
    switch(control) {
        case BYTE_LEFTARROW:
            return make_h_action(0);
        case BYTE_RIGHTARROW:
            return make_l_action(0);
        case BYTE_DOWNARROW:
            return make_j_action(0);
        case BYTE_UPARROW:
            return make_k_action(0);
        default:
            ret = make_DefaultAction("<esc>");
            ret->resolve = &ESC_action_resolve;
            return ret;
    }
}
