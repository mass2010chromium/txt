#include "editor_actions.h"

#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "editor.h"
#include "../structures/buffer.h"

// basic_actions.c
EditorAction* make_h_action(int control);  // Move left. (repeatable, no line wrap)
EditorAction* make_j_action(int control);  // Move down. (repeatable)
EditorAction* make_k_action(int control);  // Move up. (repeatable)
EditorAction* make_l_action(int control);  // Move right. (repeatable, no line wrap)
EditorAction* make_i_action(int control);  // Enter insert mode.
EditorAction* make_ESC_action(int control);    // Pop cmdstack, exit visual, etc etc.


// editing_actions.c
EditorAction* make_0_action(int control);  // Go to start of line.
EditorAction* make_DOLLAR_action(int control); // Go to end of line.

EditorAction* make_o_action(int control);  // Create a line below, and enter insert mode.
EditorAction* make_O_action(int control);  // Create a line above, and enter insert mode.
EditorAction* make_A_action(int control);  // Go to end of line and enter insert mode.

EditorAction* make_g_action(int control);  // "Go" command. (`gg` goes to start, `gt` / `gT` move between tabs)
EditorAction* make_G_action(int control);  // Go to end of buffer. (first col)

EditorAction* make_x_action(int control);  // Delete a character (repeatable), or delete the visual selection.
EditorAction* make_r_action(int control);  // Replace character(s) under cursor.
EditorAction* make_d_action(int control);  // 'dd' to delete line (repeatable), or delete based on the result of a move command.
EditorAction* make_D_action(int control);  // Shortcut for `d$`.

EditorAction* make_LEFTARROW_action(int control);   // Unindent


// visual_actions.c
EditorAction* make_v_action(int control);  // Enter visual select mode (char).
EditorAction* make_V_action(int control);  // Enter visual select mode (line).


EditorAction* make_u_action(int control);  // Undo an action. (repeatable)
EditorAction* make_y_action(int control);  // Copy text.
EditorAction* make_p_action(int control);  // Paste copied/cut text.


// search_actions.c
EditorAction* make_f_action(int control);  // Find a character forwards in line.
EditorAction* make_F_action(int control);  // Find a character backwards in line.
EditorAction* make_w_action(int control);  // Move forward by one "word", breaking on punctuation and space. (repeatable)
EditorAction* make_W_action(int control);  // Move forward by one "word", breaking on punctuation. (repeatable)

EditorAction* make_slash_action(int control);  // Search for a word across lines, forward.
EditorAction* make_question_action(int control);   // Search for a word across lines, backward.
EditorAction* make_n_action(int control);  // Repeat previous word search.
EditorAction* make_N_action(int control);  // Repeat previous word search, in the reverse direction.

EditorAction* make_m_action(int control);  // Set mark.
EditorAction* make_BACKTICK_action(int control);   // Go to mark.


// command_action.c
EditorAction* make_COLON_action(int control);  // command mode.

EditorAction* make_q_action(int control);  // Record macro.
EditorAction* make_AT_action(int control); // Execute macro.

void EditorAction_destroy(EditorAction* this) {
    free(this->value);
}

EditorAction* (*action_jump_table[256]) (int) = {0};
ActionType action_type_table[256] = {0};
Vector* /*EditorAction* */ action_stack = NULL;

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
    action_jump_table[BYTE_ESC] = &make_ESC_action;
    action_type_table[BYTE_ESC] = AT_ESCAPE;

    action_jump_table['0'] = &make_0_action;
    action_type_table['0'] = AT_MOVE;
    action_jump_table['$'] = &make_DOLLAR_action;
    action_type_table['$'] = AT_MOVE;
    action_jump_table['o'] = &make_o_action;
    action_type_table['o'] = AT_OVERRIDE;
    action_jump_table['O'] = &make_O_action;
    action_type_table['O'] = AT_OVERRIDE;
    action_jump_table['A'] = &make_A_action;
    action_type_table['A'] = AT_OVERRIDE;
    action_jump_table['x'] = &make_x_action;
    action_type_table['x'] = AT_DELETE;
    action_jump_table['r'] = &make_r_action;
    action_type_table['r'] = AT_OVERRIDE;
    action_jump_table['d'] = &make_d_action;
    action_type_table['d'] = AT_DELETE;
    action_jump_table['D'] = &make_D_action;
    action_type_table['D'] = AT_DELETE;
    action_jump_table['<'] = &make_LEFTARROW_action;
    action_type_table['<'] = AT_OVERRIDE;

    action_jump_table['v'] = &make_v_action;
    action_type_table['v'] = AT_OVERRIDE;
    action_jump_table['V'] = &make_V_action;
    action_type_table['V'] = AT_OVERRIDE;
    action_jump_table['w'] = &make_w_action;
    action_type_table['w'] = AT_MOVE;
    action_jump_table['W'] = &make_W_action;
    action_type_table['W'] = AT_MOVE;
    action_jump_table['u'] = &make_u_action;
    action_type_table['u'] = AT_UNDO;
    action_jump_table['y'] = &make_y_action;
    action_type_table['y'] = AT_OVERRIDE;
    action_jump_table['p'] = &make_p_action;
    action_type_table['p'] = AT_PASTE;
    action_jump_table['g'] = &make_g_action;
    action_type_table['g'] = AT_MOVE;
    action_jump_table['G'] = &make_G_action;
    action_type_table['G'] = AT_MOVE;
    action_jump_table['f'] = &make_f_action;
    action_type_table['f'] = AT_MOVE;
    action_jump_table['F'] = &make_F_action;
    action_type_table['F'] = AT_MOVE;
    action_jump_table['/'] = &make_slash_action;
    action_type_table['/'] = AT_MOVE;
    action_jump_table['?'] = &make_question_action;
    action_type_table['?'] = AT_MOVE;
    action_jump_table['n'] = &make_n_action;
    action_type_table['n'] = AT_MOVE;
    action_jump_table['N'] = &make_N_action;
    action_type_table['N'] = AT_MOVE;
    action_jump_table['m'] = &make_m_action;
    action_type_table['m'] = AT_OVERRIDE;
    action_jump_table['`'] = &make_BACKTICK_action;
    action_type_table['`'] = AT_MOVE;
    action_jump_table[':'] = &make_COLON_action;
    action_type_table[':'] = AT_COMMAND;
    action_jump_table['q'] = &make_q_action;
    action_type_table['q'] = AT_OVERRIDE;
    action_jump_table['@'] = &make_AT_action;
    action_type_table['@'] = AT_OVERRIDE;
    action_stack = make_Vector(10);
}

ActionType resolve_action_stack(Buffer* buf) {
    EditorAction* source = action_stack->elements[0];
    EditorContext ctx;
    ctx.start_row = Buffer_get_line_index(buf, buf->cursor_row);
    String* start_line = *Buffer_get_line(buf, buf->cursor_row);
    ctx.start_col = line_pos(start_line->data, buf->cursor_col) - start_line->data;
    ctx.jump_row = ctx.start_row;
    ctx.jump_col = ctx.start_col;
    ctx.undo_idx = buf->undo_index+1;
    ctx.sharp_move = true;
    ctx.action = AT_NONE;
    ctx.buffer = buf;

    (*source->resolve)(source, &ctx);
    // Early resolution.
    if (ctx.action == AT_COMMAND) {
        clear_action_stack();
        return ctx.action;
    }

    Buffer_clip_context(buf, &ctx);

    //interpret result
    if (ctx.action == AT_MOVE) {
        //TODO jank
        if (ctx.sharp_move) {
            buf->marks['`'].set = true;
            buf->marks['`'].row = ctx.start_row;
            buf->marks['`'].col = ctx.start_col;
        }
        RepaintType repaint = editor_move_to(ctx.jump_row, ctx.jump_col, ctx.sharp_move);
        if (repaint == RP_NONE) {
            // if not RP_NONE, the repaint was already requested. Maybe this code should be moved
            EditorMode mode = Buffer_get_mode(buf);
            if (mode == EM_VISUAL || mode == EM_VISUAL_LINE) {
                EditorContext_normalize(&ctx);
                editor_repaint(RP_LINES, &ctx);
            }
        }
    }
    else if (ctx.action == AT_OVERRIDE) {
        // Do nothing.
    }
    else if (ctx.action == AT_ESCAPE) {
        // Escape key.
        EditorMode mode = Buffer_get_mode(buf);
        if (mode == EM_VISUAL || mode == EM_VISUAL_LINE) {
            Buffer_exit_visual(buf);
        }
        Buffer_set_mode(buf, EM_NORMAL);
    }
    else {
        EditorMode mode = Buffer_get_mode(buf);
        if (mode == EM_VISUAL || mode == EM_VISUAL_LINE) {
            Buffer_exit_visual(buf);
        }
        if (ctx.action == AT_DELETE) {
            editor_new_action();
            EditorContext_normalize(&ctx);
            RepaintType repaint = Buffer_delete_range(buf, &active_copy, &ctx);
            if (ctx.start_col != -1) {
                String* s = alloc_String(10);
                String** line_p = Buffer_get_line_abs(buf, ctx.start_row);
                char* line = (*line_p)->data;
                size_t p_start = buf->left_col;
                size_t p_end = editor_width - editor_left + buf->left_col;
                size_t size = format_respect_tabspace(&s, line, 0,
                                    ctx.start_col, p_start, p_end);
                free(s);
                buf->cursor_col = size;
            }
            else {
                buf->cursor_col = 0;
            }
            editor_repaint(repaint, &ctx);
        }
        else if (ctx.action == AT_PASTE) {
            editor_new_action();
            // Oof
            editor_repaint(RP_ALL, &ctx);
        }
        else if (ctx.action == AT_UNDO) {
            int num_undo = Buffer_undo(buf, ctx.undo_idx, &ctx);
            // TODO kinda janky...
            buf->undo_index = ctx.undo_idx-1;
            if (buf->undo_index < 0) buf->undo_index = 0;
            if (num_undo > 0) {
                editor_move_to(ctx.jump_row, ctx.jump_col, true);
                editor_fix_view();
                move_to_current();
                display_current_buffer();
            }
        }
        else if (ctx.action == AT_REDO) {
            //TODO implement 
        }
    }
    clear_action_stack();
    return ctx.action;
}

Macro keybind_macros[256] = {0};
Macro* current_recording_macro = NULL;

void inplace_make_Macro(Macro* ret) {
    ret->active = 1;
    inplace_make_Vector(&ret->keypresses, 10);
}

void Macro_destroy(Macro* this) {
    this->active = 0;
    Vector_clear_free(&this->keypresses, 10);
    Vector_destroy(&this->keypresses);
}

void Macro_push(Macro* this, char c, int control) {
    Keystroke* elt = malloc(sizeof(Keystroke));
    elt->c = c;
    elt->control = control;
    Vector_push(&this->keypresses, elt);
}

/**
 * Process a char (+ control) using the current action stack.
 */
int process_action(char c, int control, Buffer* buf) {
    int retval;
    bool record_init = (current_recording_macro != NULL);
    bool record_override = true;
    if (action_stack->size != 0) {
        EditorAction* top = action_stack->elements[action_stack->size - 1];
        int res = (*top->update)(top, c, control);
        if (res == 3) {
            // failure :( 
            clear_action_stack();
            retval = -1;
            goto process_action_ret;
        }
        else if (res == 2) {
            // resolve!
            retval = resolve_action_stack(buf);
            if (current_recording_macro != NULL && !record_init) {
                // If we just started recording this time, don't record
                //      the keypress that started the recording.
                record_override = false;
            }
            goto process_action_ret;
        }
        else if (res == 1) {
            retval = 0;
            goto process_action_ret;
        }
    }
    EditorAction* new_action = make_NumberAction(c);
    if (new_action == NULL) {
        EditorAction* (*factory) (int) = action_jump_table[(int) c];
        if (factory != NULL) {
            new_action = (*factory)(control);
        }
    }
    if (new_action != NULL) {
        Vector_push(action_stack, new_action);
        if (action_stack->size != 1) {
            EditorAction* prev = action_stack->elements[action_stack->size - 2];
            prev->child = new_action;
        }
        if (new_action->update == NULL) {
            // Resolve trigger!
            resolve_action_stack(buf);
        }
        retval = 0;
    }
    else {
        retval = -1;
    }

process_action_ret:
    if (record_override && current_recording_macro != NULL) {
        print("Macro record: %c %d\n", c, control);
        Macro_push(current_recording_macro, c, control);
    }
    return retval;
}

void clear_action_stack() {
    for (int i = 0; i < action_stack->size; ++i) {
        EditorAction_destroy(action_stack->elements[i]);
        free(action_stack->elements[i]);
    }
    Vector_clear(action_stack, 10);
}

/**
 * Return a C string representing the action stack (or null if no stack).
 * DO NOT FREE THIS!
 */
char* format_action_stack() {
    static_String(ret, 10);
    
    if (current_recording_macro != NULL) {
        Strcats(&ret, "recording @");
        String_push(&ret, (char) (current_recording_macro - keybind_macros));
        String_push(&ret, ' ');
    }
    for (int i = 0; i < action_stack->size; ++i) {
        EditorAction* act = (EditorAction*) action_stack->elements[i];
        Strcats(&ret, act->value->data);
    }
    return ret->data;
}

EditorAction* make_DefaultAction(const char* s) {
    EditorAction* ret = malloc(sizeof(EditorAction));
    ret->update = NULL;
    ret->child = NULL;
    ret->resolve = NULL;
    ret->repeat = NULL;
    ret->value = make_String(s);
    ret->num_value = 0;
    return ret;
}

int NumberAction_update(EditorAction* this, char input, int control) {
    int val = input - '0';
    if (val >= 0 && val < 10) {
        String_push(&this->value, input);
        this->num_value = this->num_value * 10 + val;
        return 1;
    }
    return 0;
}

static inline bool is_stop(ActionType act) {
    return act == AT_OVERRIDE || act == AT_ESCAPE || act == AT_COMMAND;
}

void NumberAction_resolve(EditorAction* this, EditorContext* ctx) {
    if (this->child == NULL) {
        return;
    }
    if (this->child->repeat) {
        if ((*this->child->repeat)(this->child, ctx, this->num_value)) {
            return;
        }
    }
    for (int i = 0; i < this->num_value; ++i) {
        (*this->child->resolve)(this->child, ctx);
        if (is_stop(ctx->action)) break;
    }
}

EditorAction* make_NumberAction(char input) {
    int val = input - '0';
    if (val > 0 && val < 10) {  // Number action does not begin with 0.
        EditorAction* ret = make_DefaultAction("");
        ret->update = &NumberAction_update;
        ret->resolve = &NumberAction_resolve;
        String_push(&ret->value, input);
        ret->num_value = val;
        return ret;
    }
    return NULL;
}

#include "actions/basic_actions.c"
#include "actions/editing_actions.c"
#include "actions/visual_actions.c"

void w_action_resolve(EditorAction* this, EditorContext* ctx) {
    Buffer_skip_word(ctx->buffer, ctx, false);
    if (ctx->action == AT_DELETE && ctx->jump_col > 0) {
        --ctx->jump_col;
    }
    ctx->action = AT_MOVE;
}

EditorAction* make_w_action(int control) {
    EditorAction* ret = make_DefaultAction("w");
    ret->resolve = &w_action_resolve;
    return ret;
}

void W_action_resolve(EditorAction* this, EditorContext* ctx) {
    ctx->action = AT_MOVE;
    Buffer_skip_word(ctx->buffer, ctx, true);
}

EditorAction* make_W_action(int control) {
    EditorAction* ret = make_DefaultAction("W");
    ret->resolve = &W_action_resolve;
    return ret;
}

void u_action_resolve(EditorAction* this, EditorContext* ctx) {
    ctx->action = AT_UNDO;
    ctx->undo_idx -= 1;
}

EditorAction* make_u_action(int control) {
    EditorAction* ret = make_DefaultAction("u");
    ret->resolve = &u_action_resolve;
    return ret;
}

int y_action_update(EditorAction* this, char input, int control) {
    if (Strlen(this->value) == 1) {
        if (input == 'y') {
            String_push(&this->value, input);
            return 2;
        }
    }
    // lol can't accept any characters but needs a move.
    return 0;
}

void y_action_resolve(EditorAction* this, EditorContext* ctx) {
    ctx->action = AT_OVERRIDE;
    if (Strlen(this->value) == 2 && this->value->data[1] == 'y') {
        ctx->start_col = -1;
        EditorContext_normalize(ctx);
        Buffer_copy_range(ctx->buffer, &active_copy, ctx);
    }
    EditorMode mode = Buffer_get_mode(ctx->buffer);
    if (mode == EM_VISUAL || mode == EM_VISUAL_LINE) {
        ctx->jump_row = ctx->buffer->visual_row;
        ctx->jump_col = ctx->buffer->visual_col;
        if (mode == EM_VISUAL_LINE) {
            ctx->start_col = -1;
        }
        EditorContext_normalize(ctx);
        Buffer_copy_range(ctx->buffer, &active_copy, ctx);
        Buffer_exit_visual(ctx->buffer);
    }
    else if (this->child != NULL) {
        (*this->child->resolve)(this->child, ctx);
        if (is_stop(ctx->action)) return;
        if (ctx->action != AT_MOVE) return; // ERROR
        EditorContext_normalize(ctx);
        Buffer_copy_range(ctx->buffer, &active_copy, ctx);
    }
}

int y_action_repeat(EditorAction* this, EditorContext* ctx, size_t n) {
    if (Strlen(this->value) == 1) {
        // Repeated normal 'y' is nothing.
        y_action_resolve(this, ctx);
        return 1;
    }
    if (n > 0) {
        if (this->child == NULL) {
            if (Strlen(this->value) == 2 && this->value->data[1] == 'y') {
                ctx->action = AT_OVERRIDE;
                ctx->start_col = -1;
                ctx->jump_row += n - 1;
                EditorContext_normalize(ctx);
                Buffer_copy_range(ctx->buffer, &active_copy, ctx);
                return 1;
            }
        }
    }
    return 0;
}

EditorAction* make_y_action(int control) {
    EditorAction* ret = make_DefaultAction("y");
    ret->resolve = &y_action_resolve;
    ret->repeat = &y_action_repeat;
    EditorMode mode = Buffer_get_mode(current_buffer);
    if (mode == EM_VISUAL || mode == EM_VISUAL_LINE) {
        // resolve immediately.
    }
    else {
        ret->update = &y_action_update;
    }
    return ret;
}

void p_action_resolve(EditorAction* this, EditorContext* ctx) {
    ctx->action = AT_PASTE;
    Buffer_insert_copy(ctx->buffer, &active_copy, ctx);
}

EditorAction* make_p_action(int control) {
    EditorAction* ret = make_DefaultAction("p");
    ret->resolve = &p_action_resolve;
    return ret;
}

#include "actions/search_actions.c"

int m_action_update(EditorAction* this, char input, int control) {
    if (input == BYTE_ESC || input == BYTE_BACKSPACE
            || input == BYTE_ENTER || input == '`') {
        return 3;
    }
    String_push(&this->value, input);
    return 2;
}

void m_action_resolve(EditorAction* this, EditorContext* ctx) {
    ctx->action = AT_OVERRIDE;

    if (Strlen(this->value) == 2) {
        char mark_val = this->value->data[1];
        Mark* mark = &ctx->buffer->marks[(int) mark_val];
        mark->row = ctx->start_row;
        mark->col = ctx->start_col;
        mark->set = 1;
    }
}

EditorAction* make_m_action(int control) {
    EditorAction* ret = make_DefaultAction("m");
    ret->update = &m_action_update;
    ret->resolve = &m_action_resolve;
    return ret;
}

int BACKTICK_action_update(EditorAction* this, char input, int control) {
    if (input == BYTE_ESC || input == BYTE_BACKSPACE
            || input == BYTE_ENTER) {
        return 3;
    }
    String_push(&this->value, input);
    return 2;
}

void BACKTICK_action_resolve(EditorAction* this, EditorContext* ctx) {
    if (Strlen(this->value) == 2) {
        char mark_val = this->value->data[1];
        Mark* mark = &ctx->buffer->marks[(int) mark_val];
        if (mark->set) {
            ctx->jump_row = mark->row;
            ctx->jump_col = mark->col;
            ctx->action = AT_MOVE;
        }
        else {
            // TODO: throw error msg?
        }
    }
}

EditorAction* make_BACKTICK_action(int control) {
    EditorAction* ret = make_DefaultAction("`");
    ret->update = &BACKTICK_action_update;
    ret->resolve = &BACKTICK_action_resolve;
    return ret;
}

#include "actions/command_action.c"
