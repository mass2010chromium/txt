/**
 * Miscellaneous editing actions.
 * Contents:
 *  0       Start of line
 *  $       End of line
 *  o       Create a line below, and enter insert mode.
 *  A       Go to end of line and enter insert mode.
 *  g       "Go" command. (`gg` goes to start, `gt` / `gT` move between tabs)
 *  G       Go to end of buffer. (first col)
 *  x       Delete a character (repeatable), or delete the visual selection.
 *  r       Replace character(s) under cursor.
 *  d       'dd' to delete line (repeatable), or delete based on the result of a move command.
 *  D       Shortcut for `d$`.
 */

void _0_action_resolve(EditorAction* this, EditorContext* ctx) {
    ctx->action = AT_MOVE;
    ctx->jump_col = 0;
}

EditorAction* make_0_action(int control) {
    EditorAction* ret = make_DefaultAction("0");
    ret->resolve = &_0_action_resolve;
    return ret;
}

void DOLLAR_action_resolve(EditorAction* this, EditorContext* ctx) {
    ctx->action = AT_MOVE;
    Buffer* buf = ctx->buffer;
    char* line = *Buffer_get_line_abs(buf, ctx->jump_row);
    size_t line_len = strlen(line);
    size_t max_char = line_len;
    // Save newlines, but don't count them towards line length for cursor purposes.
    if (line_len > 0 && line[line_len - 1] == '\n') {
        max_char -= 1;
    }
    if (max_char >= 0) {
        max_char -= 1;
    }
    ctx->jump_col = max_char;
}

EditorAction* make_DOLLAR_action(int control) {
    EditorAction* ret = make_DefaultAction("$");
    ret->resolve = &DOLLAR_action_resolve;
    return ret;
}

void o_action_resolve(EditorAction* this, EditorContext* ctx) {
    ctx->action = AT_OVERRIDE;
    editor_new_action();
    current_mode = EM_INSERT;
    editor_move_EOL();
    begin_insert();
    editor_move_right();
    add_chr('\n');
}

EditorAction* make_o_action(int control) {
    EditorAction* ret = make_DefaultAction("o");
    ret->resolve = &o_action_resolve;
    return ret;
}

void A_action_resolve(EditorAction* this, EditorContext* ctx) {
    ctx->action = AT_OVERRIDE;
    editor_new_action();
    current_mode = EM_INSERT;
    editor_move_EOL();
    begin_insert();
    editor_move_right();
}

EditorAction* make_A_action(int control) {
    EditorAction* ret = make_DefaultAction("A");
    ret->resolve = &A_action_resolve;
    return ret;
}

int g_action_update(EditorAction* this, char input, int control) {
    switch(input) {
        case 'g':
        case 't':
        case 'T':
            String_push(&this->value, input);
            return 2;
        default:
            return 3;
    }
}

void g_action_resolve(EditorAction* this, EditorContext* ctx) {
    if (this->child == NULL) {
        if (Strlen(this->value) == 2) {
            switch(this->value->data[1]) {
                case 'g':
                    ctx->action = AT_MOVE;
                    ctx->jump_row = 0;
                    ctx->jump_col = 0;
                    return;
                case 't':
                    // AT_COMMAND since we are screwing with the current buffer
                    // so we don't want any potential postproc code running
                    ctx->action = AT_COMMAND;
                    current_buffer_idx = (current_buffer_idx + 1) % buffers.size;
                    editor_switch_buffer(current_buffer_idx);
                    display_current_buffer();
            }
        }
    }
    // TODO implement repeat action override for goto tab
}

EditorAction* make_g_action(int control) {
    EditorAction* ret = make_DefaultAction("g");
    ret->update = &g_action_update;
    ret->resolve = &g_action_resolve;
    return ret;
}

void G_action_resolve(EditorAction* this, EditorContext* ctx) {
    ctx->action = AT_MOVE;
    ctx->jump_row = Buffer_get_num_lines(ctx->buffer);
    ctx->jump_col = 0;
}

EditorAction* make_G_action(int control) {
    EditorAction* ret = make_DefaultAction("G");
    ret->resolve = &G_action_resolve;
    return ret;
}

void x_action_resolve(EditorAction* this, EditorContext* ctx) {
    ctx->action = AT_DELETE;
    Buffer* buf = ctx->buffer;
    EditorMode mode = Buffer_get_mode(buf);
    if (mode == EM_VISUAL) {
        ctx->jump_col = buf->visual_col;
        ctx->jump_row = buf->visual_row;
    }
    else if (mode == EM_VISUAL_LINE) {
        ctx->start_col = -1;    // Delete the whole line
        ctx->jump_col = 0;
        ctx->jump_row = buf->visual_row;
    }
}

int x_action_repeat(EditorAction* this, EditorContext* ctx, size_t n) {
    if (n > 0) {
        ctx->action = AT_DELETE;
        ctx->jump_col += n - 1;
        return 1;
    }
    return 0;
}

EditorAction* make_x_action(int control) {
    EditorAction* ret = make_DefaultAction("x");
    ret->resolve = &x_action_resolve;
    ret->repeat = &x_action_repeat;
    return ret;
}

int r_action_update(EditorAction* this, char input, int control) {
    if (input == BYTE_ESC) {
        return 3;
    }
    String_push(&this->value, input);
    return 2;
}


void r_action_resolve(EditorAction* this, EditorContext* ctx) {
    Buffer* buf = ctx->buffer;
    EditorMode mode = Buffer_get_mode(buf);
    if (Strlen(this->value) == 2) {
        ctx->action = AT_OVERRIDE;
        char replace_ch = this->value->data[1];
        if (mode == EM_VISUAL || mode == EM_VISUAL_LINE) {
            // TODO: multi replace
        }
        else {
            editor_new_action();
            Edit* edit = malloc(sizeof(Edit));
            char** line_p = Buffer_get_line_abs(buf, ctx->start_row);
            edit->undo_index = ctx->undo_idx;
            edit->start_row = ctx->start_row;
            edit->start_col = ctx->start_col;
            edit->old_content = strndup(*line_p + ctx->start_col, 1);
            edit->new_content = alloc_String(1);
            *(*line_p + ctx->start_col) = replace_ch;
            String_push(&edit->new_content, replace_ch);
            Buffer_push_undo(buf, edit);
            editor_repaint(RP_LINES, ctx);
        }
    }
}

int r_action_repeat(EditorAction* this, EditorContext* ctx, size_t n) {
    r_action_resolve(this, ctx);
    return 1;
}

EditorAction* make_r_action(int control) {
    EditorAction* ret = make_DefaultAction("r");
    ret->update = &r_action_update;
    ret->resolve = &r_action_resolve;
    ret->repeat = &r_action_repeat;
    return ret;
}

int d_action_update(EditorAction* this, char input, int control) {
    if (Strlen(this->value) == 1) {
        if (input == 'd') {
            String_push(&this->value, input);
            return 2;
        }
        else if (input == 'u') {
            String_push(&this->value, input);
            return 1;
        }
    }
    // Note: this is only for `du`; though ig it works for `di` too eventually
    else if (Strlen(this->value) == 2) {
        String_push(&this->value, input);
        return 2;
    }
    return 0;
}

void d_action_resolve(EditorAction* this, EditorContext* ctx) {
    if (this->child == NULL) {
        if (Strlen(this->value) == 2 && this->value->data[1] == 'd') {
            ctx->action = AT_DELETE;
            ctx->start_col = -1;
            return;
        }
        if (Strlen(this->value) == 3) {
            if (this->value->data[1] == 'u') {
                Buffer_search_char(ctx->buffer, ctx, this->value->data[2], true);
                if (ctx->jump_col > ctx->start_col) {
                    ctx->action = AT_DELETE;
                    ctx->jump_col -= 1;
                }
                else {
                    ctx->action = AT_ESCAPE;
                }
                return;
            }
        }
    }
    // TODO implement move-delete
    else {
        ctx->action = AT_DELETE;    // Signal intent to delete.
        (*this->child->resolve)(this->child, ctx);
        if (is_stop(ctx->action)) return;
        if (ctx->action != AT_MOVE) return; // ERROR
        ctx->action = AT_DELETE;
    }
}

int d_action_repeat(EditorAction* this, EditorContext* ctx, size_t n) {
    if (n > 0) {
        if (this->child == NULL) {
            if (Strlen(this->value) == 2 && this->value->data[1] == 'd') {
                ctx->action = AT_DELETE;
                ctx->start_col = -1;
                ctx->jump_row += n - 1;
                return 1;
            }
        }
    }
    return 0;
}

EditorAction* make_d_action(int control) {
    EditorAction* ret = make_DefaultAction("d");
    ret->update = &d_action_update;
    ret->resolve = &d_action_resolve;
    ret->repeat = &d_action_repeat;
    return ret;
}

void D_action_resolve(EditorAction* this, EditorContext* ctx) {
    DOLLAR_action_resolve(this, ctx);
    ctx->action = AT_DELETE;
}

EditorAction* make_D_action(int control) {
    EditorAction* ret = make_DefaultAction("D");
    ret->resolve = &D_action_resolve;
    return ret;
}
