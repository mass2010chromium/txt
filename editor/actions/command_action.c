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
        editor_make_buffer(rest);
        editor_switch_buffer(buffers.size - 1);
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
        Vector* /*EditorAction* */ save_action_stack = action_stack;
        action_stack = make_Vector(10);

        editor_new_action();
        editor_display = 0;
        editor_macro_mode = 1;

        Buffer* buf = ctx->buffer;
        EditorMode mode = Buffer_get_mode(buf);
        if (mode == EM_VISUAL) {
            ctx->jump_row = buf->visual_row;
            Buffer_exit_visual(buf);
        }
        else if (mode == EM_VISUAL_LINE) {
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
            for (char* it = rest; *it; ++it) {
                print("echo char %c\n", *it);
                process_input(*it, 0);

            }
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
        editor_repaint(RP_LINES, ctx);
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

EditorAction* make_COLON_action() {
    EditorAction* ret = make_DefaultAction(":");
    ret->update = &COLON_action_update;
    ret->resolve = &COLON_action_resolve;
    return ret;
}
