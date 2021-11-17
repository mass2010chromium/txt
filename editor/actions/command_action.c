
void close_buffer() {
    editor_close_buffer(current_buffer_idx);
    if (buffers.size == 0) {
        current_mode = EM_QUIT;
    }
    else {
        display_current_buffer();
        String_clear(bottom_bar_info);
        Strcats(&bottom_bar_info, "-- Editing: ");
        Strcats(&bottom_bar_info, current_buffer->name);
        Strcats(&bottom_bar_info, " --");
        display_bottom_bar(bottom_bar_info->data, NULL);
    }
}

void save_buffer() {
    Buffer_save(current_buffer);
    String_clear(bottom_bar_info);
    Strcats(&bottom_bar_info, "-- File saved: ");
    Strcats(&bottom_bar_info, current_buffer->name);
    Strcats(&bottom_bar_info, " --");
    display_bottom_bar(bottom_bar_info->data, NULL);
}

void Macro_exec(Macro* macro) {
    for (size_t i = 0; i < macro->keypresses.size; ++i) {
        Keystroke* keypress = macro->keypresses.elements[i];
        print("echo char %c %d\n", keypress->c, keypress->control);
        process_input(keypress->c, keypress->control);
    }
}

void process_command(char* command, EditorContext* ctx) {
    if (strcmp(command, "q") == 0) {
        close_buffer();
        return;
    }
    if (strcmp(command, "w") == 0) {
        save_buffer();
        return;
    }
    if (strcmp(command, "wq") == 0 || strcmp(command, "qw") == 0) {
        save_buffer();
        close_buffer();
        return;
    }
    char* rest;
    if (strncmp(command, "tabnew ", 7) == 0) {
        rest = command + 7;
        editor_make_buffer(rest, editor_get_buffer_idx() + 1);
        editor_switch_buffer(editor_get_buffer_idx() + 1);
        display_current_buffer();
        String_clear(bottom_bar_info);
        Strcats(&bottom_bar_info, "-- Opened file: ");
        Strcats(&bottom_bar_info, current_buffer->name);
        Strcats(&bottom_bar_info, " --");
        display_bottom_bar(bottom_bar_info->data, NULL);
        // display_top_bar();
        return;
    }
    if (strncmp(command, "norm", 4) == 0) {
        rest = command + 4;
        Macro macro;
        inplace_make_Macro(&macro);
        for (char* it = rest; *it; ++it) {
            Macro_push(&macro, *it, 0);
        }
        Vector* /*EditorAction* */ save_action_stack = action_stack;
        action_stack = make_Vector(10);
    
        editor_new_action();
        editor_display = 0;
        editor_macro_mode = 1;
    
        Buffer* buf = ctx->buffer;
        EditorMode mode = Buffer_get_mode(buf);
        if (mode == EM_VISUAL || mode == EM_VISUAL_LINE) {
            ctx->jump_row = buf->visual_row;
            Buffer_exit_visual(buf);
        }
        else {
            ctx->jump_row = ctx->start_row;
        }
        EditorContext_normalize(ctx);
        for (size_t row = ctx->start_row; row <= ctx->jump_row; ++row) {
            print("Norm row %ld\n", row);
            editor_move_to(row, 0);
            Macro_exec(&macro);
            if (current_mode == EM_INSERT) {
                end_insert();
                current_mode = EM_NORMAL;
                // TODO update data structures
                display_bottom_bar("-- NORMAL --", NULL);
                // Match vim behavior when exiting insert mode.
                editor_move_left();
                editor_align_tab();
                Buffer_set_mode(current_buffer, EM_NORMAL);
            }
            EditorMode mode = Buffer_get_mode(buf);
            if (mode == EM_VISUAL || mode == EM_VISUAL_LINE) {
                Buffer_exit_visual(buf);
            }
            clear_action_stack();
        }
    
        Vector_destroy(action_stack);
        free(action_stack);
        action_stack = save_action_stack;
        
        editor_display = 1;
        editor_macro_mode = 0;
        Macro_destroy(&macro);
        editor_repaint(RP_ALL, ctx);
        return;
    }
    char* scan_start = command;
    char* scan_end = NULL;
    errno = 0;
    long int result = strtol(scan_start, &scan_end, 10);
    if (errno == 0 && result >= 0 && *scan_end == 0) {
        editor_move_to(result, 0);
    }
}

int COLON_action_update(EditorAction* this, char input, int control) {
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

void COLON_action_resolve(EditorAction* this, EditorContext* ctx) {
    if (this->child != NULL) {
        (*this->child->resolve)(this->child, ctx);
        return;
    }
    ctx->action = AT_COMMAND;
    if (Strlen(this->value) > 1) {
        process_command(this->value->data + 1, ctx);
    }
}

EditorAction* make_COLON_action(int control) {
    EditorAction* ret = make_DefaultAction(":");
    ret->update = &COLON_action_update;
    ret->resolve = &COLON_action_resolve;
    return ret;
}

int q_action_update(EditorAction* this, char input, int control) {
    if (input == BYTE_ESC || input == BYTE_BACKSPACE
            || input == BYTE_ENTER) {
        return 3;
    }
    String_push(&this->value, input);
    return 2;
}

void q_action_resolve(EditorAction* this, EditorContext* ctx) {
    ctx->action = AT_OVERRIDE;
    
    if (Strlen(this->value) == 2) {
        char record_val = this->value->data[1];
        Macro* macro = &keybind_macros[(int) record_val];
        if (macro->active) {
            Macro_destroy(macro);
        }
        inplace_make_Macro(macro);
        current_recording_macro = macro;
        print("New recording: %c\n", record_val);
    }
}

EditorAction* make_q_action(int control) {
    EditorAction* ret = make_DefaultAction("q");
    if (current_recording_macro != NULL) {
        print("Reset recording\n");
        current_recording_macro = NULL;
    }
    else {
        ret->update = &q_action_update;
    }
    ret->resolve = &q_action_resolve;
    return ret;
}

int AT_action_update(EditorAction* this, char input, int control) {
    if (input == BYTE_ESC || input == BYTE_BACKSPACE
            || input == BYTE_ENTER) {
        return 3;
    }
    String_push(&this->value, input);
    return 2;
}

int AT_action_repeat(EditorAction* this, EditorContext* ctx, size_t count) {
    if (Strlen(this->value) == 2) {
        char record_val = this->value->data[1];
        Macro* macro = &keybind_macros[(int) record_val];
        if (macro == current_recording_macro) {
            // ERROR lol
            ctx->action = AT_OVERRIDE;
        }
        else if (macro->active) {
            Vector* /*EditorAction* */ save_action_stack = action_stack;
            action_stack = make_Vector(10);
        
            editor_new_action();
            editor_display = 0;
            editor_macro_mode = 1;
        
            for (int i = 0; i < count; ++i) {
                Macro_exec(macro);
            }
        
            Vector_destroy(action_stack);
            free(action_stack);
            action_stack = save_action_stack;
            
            editor_display = 1;
            editor_macro_mode = 0;
            editor_fix_view();
            move_to_current();
            display_current_buffer();
        }
        ctx->action = AT_OVERRIDE;
        return 1;
    }
    return 0;
}

void AT_action_resolve(EditorAction* this, EditorContext* ctx) {
    AT_action_repeat(this, ctx, 1);
}

EditorAction* make_AT_action(int control) {
    EditorAction* ret = make_DefaultAction("@");
    ret->update = &AT_action_update;
    ret->resolve = &AT_action_resolve;
    ret->repeat = &AT_action_repeat;
    return ret;
}

