#include "editor_actions.h"

#include <stdlib.h>
#include <string.h>

#include "editor.h"
#include "buffer.h"

EditorAction* make_h_action();
EditorAction* make_j_action();
EditorAction* make_k_action();
EditorAction* make_l_action();
EditorAction* make_i_action();
EditorAction* make_o_action();

EditorAction* make_u_action();
EditorAction* make_p_action();

EditorAction* make_g_action();
EditorAction* make_G_action();

EditorAction* make_x_action();
EditorAction* make_d_action();

EditorAction* make_A_action();

EditorAction* make_DOLLAR_action();

void EditorAction_destroy(EditorAction* this) {
    free(this->value);
}

EditorAction* (*action_jump_table[256]) (void) = {0};
ActionType action_type_table[256] = {0};
Vector /*EditorAction* */ action_stack = {0};

void init_actions() {
    action_jump_table['h'] = &make_h_action;
    action_type_table['h'] = AT_MOVE;
    action_jump_table['j'] = &make_j_action;
    action_type_table['j'] = AT_MOVE;
    action_jump_table[BYTE_ENTER] = &make_j_action;
    action_type_table[BYTE_ENTER] = AT_MOVE;
    action_jump_table['k'] = &make_k_action;
    action_type_table['k'] = AT_MOVE;
    action_jump_table['l'] = &make_l_action;
    action_type_table['l'] = AT_MOVE;
    action_jump_table['i'] = &make_i_action;
    action_type_table['i'] = AT_OVERRIDE;
    action_jump_table['o'] = &make_o_action;
    action_type_table['o'] = AT_OVERRIDE;
    action_jump_table['u'] = &make_u_action;
    action_type_table['u'] = AT_UNDO;
    action_jump_table['p'] = &make_p_action;
    action_type_table['p'] = AT_PASTE;
    action_jump_table['g'] = &make_g_action;
    action_type_table['g'] = AT_MOVE;
    action_jump_table['G'] = &make_G_action;
    action_type_table['G'] = AT_MOVE;
    action_jump_table['x'] = &make_x_action;
    action_type_table['x'] = AT_DELETE;
    action_jump_table['d'] = &make_d_action;
    action_type_table['d'] = AT_DELETE;
    action_jump_table['A'] = &make_A_action;
    action_type_table['A'] = AT_OVERRIDE;
    action_jump_table['$'] = &make_DOLLAR_action;
    action_type_table['$'] = AT_MOVE;
    inplace_make_Vector(&action_stack, 10);
}

void resolve_action_stack() {
    EditorAction* source = action_stack.elements[0];
    EditorContext ctx;
    Buffer* buf = current_buffer;
    ctx.start_row = Buffer_get_line_index(buf, buf->cursor_row);
    char* start_line = *Buffer_get_line(buf, buf->cursor_row);
    ctx.start_col = line_pos(start_line, buf->cursor_col) - start_line;
    ctx.jump_row = ctx.start_row;
    ctx.jump_col = ctx.start_col;
    ctx.undo_idx = buf->undo_index+1;
    ctx.action = AT_NONE;
    ctx.buffer = buf;

    editor_new_action();
    (*source->resolve)(source, &ctx);
    if (ctx.jump_col < 0) {
        ctx.jump_col = 0;
    }
    if (ctx.jump_row < 0) {
        ctx.jump_row = 0;
    }
    if (ctx.jump_row >= Buffer_get_num_lines(buf)) {
        ctx.jump_row = Buffer_get_num_lines(buf);
    }
    else {
        char* linep = *Buffer_get_line_abs(buf, ctx.jump_row);
        size_t max_col = strlen(linep);
        if (ctx.jump_col > max_col) {
            ctx.jump_col = max_col;
        }
    }

    //interpret result
    if (ctx.action == AT_MOVE) {
        //TODO jank
        --buf->undo_index;
        editor_move_to(ctx.jump_row, ctx.jump_col);
    }
    else if (ctx.action == AT_DELETE) {
        RepaintType repaint = Buffer_delete_range(buf, &active_copy, &ctx);
        editor_repaint(repaint, &ctx);
    }
    else if (ctx.action == AT_PASTE) {
        editor_repaint(RP_ALL, &ctx);
    }
    else if (ctx.action == AT_UNDO) {
        int num_undo = Buffer_undo(buf, ctx.undo_idx);
        buf->undo_index = ctx.undo_idx-1;
        if (buf->undo_index < 0) buf->undo_index = 0;
        if (num_undo > 0) {
            editor_fix_view();
            display_current_buffer();
            move_to_current();
        }
    }
    else if (ctx.action == AT_REDO) {
        //TODO implement 
    }
    else if (ctx.action == AT_OVERRIDE) {
        // Do nothing.
    }
    clear_action_stack();
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
            resolve_action_stack();
            return 0;
        }
        else if (res == 1) {
            return 0;
        }
    }
    EditorAction* new_action = make_NumberAction(c);
    if (new_action == NULL) {
        EditorAction* (*factory) (void) = action_jump_table[c];
        if (factory != NULL) {
            new_action = (*factory)();
        }
    }
    if (new_action != NULL) {
        Vector_push(&action_stack, new_action);
        if (action_stack.size != 1) {
            EditorAction* prev = action_stack.elements[action_stack.size - 2];
            prev->child = new_action;
        }
        if (new_action->update == NULL) {
            // Resolve trigger!
            resolve_action_stack();
            return 0;
        }
        return 0;
    }
    return -1;
}

void clear_action_stack() {
    for (int i = 0; i < action_stack.size; ++i) {
        EditorAction_destroy(action_stack.elements[i]);
        free(action_stack.elements[i]);
    }
    Vector_clear(&action_stack, 10);
}

int NumberAction_update(EditorAction* this, char input, int control) {
    int val = input - '0';
    if (val >= 0 && val < 10) {
        *this->num_value = *this->num_value * 10 + val;
        return 1;
    }
    return 0;
}

void NumberAction_resolve(EditorAction* this, EditorContext* ctx) {
    if (this->child == NULL) {
        return;
    }
    for (int i = 0; i < *this->num_value; ++i) {
        // TODO: repeating
        (*this->child->resolve)(this->child, ctx);
        if (ctx->action == AT_OVERRIDE) break;
    }
}

EditorAction* make_NumberAction(char input) {
    int val = input - '0';
    if (val >= 0 && val < 10) {
        EditorAction* ret = malloc(sizeof(EditorAction));
        ret->update = &NumberAction_update;
        ret->resolve = &NumberAction_resolve;
        ret->child = NULL;
        ret->num_value = malloc(sizeof(size_t));
        *ret->num_value = val;
        return ret;
    }
    return NULL;
}

void j_action_resolve(EditorAction* this, EditorContext* ctx) {
    ctx->action = AT_MOVE;
    ctx->jump_row += 1;
    ctx->jump_col = ctx->buffer->natural_col;
}

EditorAction* make_j_action() {
    EditorAction* ret = malloc(sizeof(EditorAction));
    ret->update = NULL;
    ret->resolve = &j_action_resolve;
    ret->child = NULL;
    ret->value = make_String("j");
    return ret;
}

void k_action_resolve(EditorAction* this, EditorContext* ctx) {
    ctx->action = AT_MOVE;
    ctx->jump_row -= 1;
    ctx->jump_col = ctx->buffer->natural_col;
}

EditorAction* make_k_action() {
    EditorAction* ret = malloc(sizeof(EditorAction));
    ret->update = NULL;
    ret->resolve = &k_action_resolve;
    ret->child = NULL;
    ret->value = make_String("k");
    return ret;
}

void h_action_resolve(EditorAction* this, EditorContext* ctx) {
    ctx->action = AT_MOVE;
    ctx->jump_col -= 1;
}

EditorAction* make_h_action() {
    EditorAction* ret = malloc(sizeof(EditorAction));
    ret->update = NULL;
    ret->resolve = &h_action_resolve;
    ret->child = NULL;
    ret->value = make_String("h");
    return ret;
}

void l_action_resolve(EditorAction* this, EditorContext* ctx) {
    ctx->action = AT_MOVE;
    ctx->jump_col += 1;
}

EditorAction* make_l_action() {
    EditorAction* ret = malloc(sizeof(EditorAction));
    ret->update = NULL;
    ret->resolve = &l_action_resolve;
    ret->child = NULL;
    ret->value = make_String("l");
    return ret;
}

void i_action_resolve(EditorAction* this, EditorContext* ctx) {
    ctx->action = AT_OVERRIDE;
    current_mode = EM_INSERT;
    begin_insert();
    editor_align_tab();
}

EditorAction* make_i_action() {
    EditorAction* ret = malloc(sizeof(EditorAction));
    ret->update = NULL;
    ret->resolve = &i_action_resolve;
    ret->child = NULL;
    ret->value = make_String("i");
    return ret;
}

void o_action_resolve(EditorAction* this, EditorContext* ctx) {
    ctx->action = AT_OVERRIDE;
    current_mode = EM_INSERT;
    editor_move_EOL();
    begin_insert();
    editor_move_right();
    add_chr('\n');
}

EditorAction* make_o_action() {
    EditorAction* ret = malloc(sizeof(EditorAction));
    ret->update = NULL;
    ret->resolve = &o_action_resolve;
    ret->child = NULL;
    ret->value = make_String("o");
    return ret;
}

void u_action_resolve(EditorAction* this, EditorContext* ctx) {
    ctx->action = AT_UNDO;
    ctx->undo_idx -= 1;
}

EditorAction* make_u_action() {
    EditorAction* ret = malloc(sizeof(EditorAction));
    ret->update = NULL;
    ret->resolve = &u_action_resolve;
    ret->child = NULL;
    ret->value = make_String("u");
    return ret;
}

void p_action_resolve(EditorAction* this, EditorContext* ctx) {
    ctx->action = AT_PASTE;
    Buffer_insert_copy(ctx->buffer, &active_copy, ctx);
}

EditorAction* make_p_action() {
    EditorAction* ret = malloc(sizeof(EditorAction));
    ret->update = NULL;
    ret->resolve = &p_action_resolve;
    ret->child = NULL;
    ret->value = make_String("p");
    return ret;
}

int g_action_update(EditorAction* this, char input, int control) {
    if (input == 'g') {
        String_push(&this->value, 'g');
        return 2;
    }
    return 0;
}

void g_action_resolve(EditorAction* this, EditorContext* ctx) {
    if (this->child == NULL) {
        if (strcmp(this->value->data, "gg") == 0) {
            ctx->action = AT_MOVE;
            ctx->jump_row = 0;
            ctx->jump_col = 0;
        }
    }
    // TODO implement move-delete
    else {
        (*this->child->resolve)(this->child, ctx);
    }
}

EditorAction* make_g_action() {
    EditorAction* ret = malloc(sizeof(EditorAction));
    ret->update = &g_action_update;
    ret->resolve = &g_action_resolve;
    ret->child = NULL;
    ret->value = make_String("g");
    return ret;
}

void G_action_resolve(EditorAction* this, EditorContext* ctx) {
    ctx->action = AT_MOVE;
    ctx->jump_row = Buffer_get_num_lines(ctx->buffer);
    ctx->jump_col = 0;
}

EditorAction* make_G_action() {
    EditorAction* ret = malloc(sizeof(EditorAction));
    ret->update = NULL;
    ret->resolve = &G_action_resolve;
    ret->child = NULL;
    ret->value = make_String("G");
    return ret;
}

void x_action_resolve(EditorAction* this, EditorContext* ctx) {
    ctx->action = AT_DELETE;
    ++ctx->jump_col;
}

EditorAction* make_x_action() {
    EditorAction* ret = malloc(sizeof(EditorAction));
    ret->update = NULL;
    ret->resolve = &x_action_resolve;
    ret->child = NULL;
    ret->value = make_String("x");
    return ret;
}


int d_action_update(EditorAction* this, char input, int control) {
    if (input == 'd') {
        String_push(&this->value, 'd');
        return 2;
    }
    return 0;
}

void d_action_resolve(EditorAction* this, EditorContext* ctx) {
    if (this->child == NULL) {
        if (strcmp(this->value->data, "dd") == 0) {
            ctx->action = AT_DELETE;
            ctx->start_col = -1;    // Delete the whole line
            ctx->jump_row += 1;
            ctx->jump_col = 0;
        }
    }
    // TODO implement move-delete
    else {
        (*this->child->resolve)(this->child, ctx);
        if (ctx->action == AT_OVERRIDE) return;
        if (ctx->action != AT_MOVE) return; // ERROR
        ctx->action = AT_DELETE;
    }
}

EditorAction* make_d_action() {
    EditorAction* ret = malloc(sizeof(EditorAction));
    ret->update = &d_action_update;
    ret->resolve = &d_action_resolve;
    ret->child = NULL;
    ret->value = make_String("d");
    return ret;
}

void A_action_resolve(EditorAction* this, EditorContext* ctx) {
    ctx->action = AT_OVERRIDE;
    current_mode = EM_INSERT;
    editor_move_EOL();
    begin_insert();
    editor_move_right();
}

EditorAction* make_A_action() {
    EditorAction* ret = malloc(sizeof(EditorAction));
    ret->update = NULL;
    ret->resolve = &A_action_resolve;
    ret->child = NULL;
    ret->value = make_String("A");
    return ret;
}

void DOLLAR_action_resolve(EditorAction* this, EditorContext* ctx) {
    ctx->action = AT_MOVE;
    editor_move_EOL();
}

EditorAction* make_DOLLAR_action() {
    EditorAction* ret = malloc(sizeof(EditorAction));
    ret->update = NULL;
    ret->resolve = &DOLLAR_action_resolve;
    ret->child = NULL;
    ret->value = make_String("$");
    return ret;
}
