/**
 * Visual mode related actions.
 * Contents:
 *  v       visual character mode
 *  V       visual line mode
 */

void v_action_resolve(EditorAction* this, EditorContext* ctx) {
    ctx->action = AT_OVERRIDE;
    Buffer* buf = ctx->buffer;
    // TODO: num->v
    EditorMode mode = Buffer_get_mode(buf);
    if (mode == EM_VISUAL) {
        // exit visual mode.
        Buffer_exit_visual(buf);
        return;
    }
    Buffer_set_mode(buf, EM_VISUAL);
    if (mode == EM_NORMAL) {
        buf->visual_row = ctx->jump_row;
        buf->visual_col = ctx->jump_col;
        editor_repaint(RP_LINES, ctx);
        return;
    }
    ctx->jump_row = buf->visual_row;
    editor_repaint(RP_LINES, ctx);
}

EditorAction* make_v_action() {
    EditorAction* ret = make_DefaultAction("v");
    ret->resolve = &v_action_resolve;
    return ret;
}


void V_action_resolve(EditorAction* this, EditorContext* ctx) {
    ctx->action = AT_OVERRIDE;
    Buffer* buf = ctx->buffer;
    // TODO: num->v
    EditorMode mode = Buffer_get_mode(buf);
    if (mode == EM_VISUAL_LINE) {
        // exit visual mode.
        Buffer_set_mode(buf, EM_NORMAL);
        ctx->jump_row = buf->visual_row;
        editor_repaint(RP_LINES, ctx);
        return;
    }
    Buffer_set_mode(buf, EM_VISUAL_LINE);
    if (mode == EM_NORMAL) {
        buf->visual_row = ctx->jump_row;
        buf->visual_col = ctx->jump_col;
        editor_repaint(RP_LINES, ctx);
        return;
    }
    ctx->jump_row = buf->visual_row;
    editor_repaint(RP_LINES, ctx);
}

EditorAction* make_V_action() {
    EditorAction* ret = make_DefaultAction("V");
    ret->resolve = &V_action_resolve;
    return ret;
}
