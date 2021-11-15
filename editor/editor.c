#include <stdlib.h>
#include <termios.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "editor.h"
#include "editor_actions.h"
#include "utils.h"
#include "debugging.h"

int TAB_WIDTH = 4;
bool PRESERVE_INDENT = true;
bool EXPAND_TAB = true;

bool SCREEN_WRITE = true;

Copy active_copy = {0};

struct winsize window_size = {0};

Buffer* current_buffer = NULL;

Vector/*Buffer* */ buffers = {0};
String* command_buffer = NULL;
GapBuffer active_insert = {0};
size_t editor_top = 0;
size_t editor_left = 0;
size_t editor_bottom = 0;
bool editor_display = 0;
bool editor_macro_mode = 0;

String* bottom_bar_info = NULL;

const char* EDITOR_MODE_STR[5] = {
    "NORMAL",
    "INSERT",
    "COMMAND",
    "VISUAL",
    "VISUAL LINE",
};

void process_input(char input, int control) {
    if (current_mode == EM_INSERT) {
        if (current_recording_macro != NULL) {
            Macro_push(current_recording_macro, input, control);
        }
        display_bottom_bar("-- INSERT --", NULL);
        if (input == BYTE_ESC) {
            switch(control) {
                case BYTE_UPARROW:
                    editor_move_up();
                    break;
                case BYTE_DOWNARROW:
                    editor_move_down();
                    break;
                case BYTE_LEFTARROW:
                    editor_move_left();
                    break;
                case BYTE_RIGHTARROW:
                    editor_move_right();
                    break;
                default:
                    end_insert();
                    current_mode = EM_NORMAL;
                    // TODO update data structures
                    display_bottom_bar("-- NORMAL --", NULL);
                    // Match vim behavior when exiting insert mode.
                    editor_move_left();
                    editor_align_tab();
                    Buffer_set_mode(current_buffer, EM_NORMAL);
            }
            move_to_current();
            return;
        }
        if (input == BYTE_BACKSPACE) { editor_backspace(); }
        else { add_chr(input); }
        move_to_current();
    }
    else if (current_mode == EM_NORMAL) {
        int res = process_action(input, control);
        if (res == -1) {
            clear_action_stack();
        }
        else if (res != AT_COMMAND) {
            move_to_current();
            String_clear(bottom_bar_info);
            Strcats(&bottom_bar_info, "-- ");
            Strcats(&bottom_bar_info, EDITOR_MODE_STR[Buffer_get_mode(current_buffer)]);
            Strcats(&bottom_bar_info, " -- ");
            Strcats(&bottom_bar_info, format_action_stack());
            display_bottom_bar(bottom_bar_info->data, NULL);
        }
    }
}

static inline ssize_t _write(const char* string, size_t n) {
    if (SCREEN_WRITE && editor_display) {
        return write(STDOUT_FILENO, string, n);
    }
    return 0;
}

/**
 * Callback function that gets called whenever the terminal size changes.
 * Updates the window_size variable to reflect changes made to the window size,
 * then also updates the editor_bottom variable accordingly.
 */
void editor_window_size_change() {
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &window_size);
    editor_bottom = window_size.ws_row - 1;
}

/**
 * Start a new editor action for undo purposes.
 * Increments the undo counter.
 */
void editor_new_action() {
    if (!editor_macro_mode) {
        current_buffer->undo_index += 1;
    }
}

/**
 * Performs the latest undo action contained in the current buffer's
 * undo list, then decrements the undo counter and re-displays the buffer
 * to reflect the changes.
 * Wraps Buffer->undo.
 *
 * Return:
 *   Number of actions undone (this is kinda meaningless tho)
 */
int editor_undo() {
    int num_undo = Buffer_undo(current_buffer, current_buffer->undo_index);
    if (num_undo > 0) {
        current_buffer->undo_index -= 1;
        display_current_buffer();
    }
    return num_undo;
}

const char* CLEAR_LINE = "\033[0K";
/**
 * Clear the current line of text.
 * I'm doing this by writing the empty space char instead of using
 * control chars in an attempt to make it work on tmux wsl 20.04.
 */
void clear_line() {
    _write(CLEAR_LINE, strlen(CLEAR_LINE));
}

/**
 * Clear the entire screen by writing a control character.
 */
void clear_screen() {
    _write("\033[2J", 4);
}

const char* SET_HIGHLIGHT = "\033[7m";
const char* RESET_HIGHLIGHT = "\033[0m";

/**
 * Adapted from https://stackoverflow.com/questions/50884685/how-to-get-cursor-position-in-c-using-ansi-code
 * See: https://en.wikipedia.org/wiki/ANSI_escape_code (Device Status Report)
 *
 * Writes the numbers to the given pointers.
 *
 * Returns success or fail: 0 for success, 1 for failure.
 */
int get_cursor_pos(size_t *y, size_t *x) {
    char buf[30] = {0};
    int ret, i;
    size_t pow;
    char ch;
    *y = 0;
    *x = 0;
   
    _write("\033[6n", 4);
   
    for ( i = 0, ch = 0; ch != 'R'; i++ ) {
        ret = read(STDIN_FILENO, &ch, 1);
        if (ret == -1) {
            if (errno == EAGAIN) {
                i -= 1;
                continue;
            }
        }
        if (ret == 0) {
            return 1;
        }
        buf[i] = ch;
    }
    if (i < 2) {
        return 1;
    }
    // The terminal returns a `;` separated string of (y, x)
    // (row, col). It is one-indexed but we want zero-indexed.
    for ( i -= 2, pow = 1; buf[i] != ';'; i--, pow *= 10)
        *x = *x + ( buf[i] - '0' ) * pow;
   
    for ( i-- , pow = 1; buf[i] != '['; i--, pow *= 10)
        *y = *y + ( buf[i] - '0' ) * pow;
    // Zero indexed;
    *x -= 1;
    *y -= 1;
    return 0;
}

/**
 * Move cursor to positions (y, x) (row, col).
 * Zero indexed.
 */
void move_cursor(size_t y, size_t x) {
    print("move raw %ld %ld\n", y, x);
    char buf[60] = {0};
    sprintf(buf, "\033[%lu;%luH", y+1, x+1);
    _write(buf, strlen(buf));
}

/**
 * Move the cursor to the current (row, col) of the buffer.
 */
void move_to_current() {
    if (SCREEN_WRITE && editor_display) {
        print("move current %ld %ld\n", current_buffer->cursor_row, current_buffer->cursor_col);
        move_cursor(current_buffer->cursor_row + editor_top, current_buffer->cursor_col + editor_left);
    }
}

/**
 * Suppose I am at position <start>, and I press tab.
 * Where should I end up?
 */
size_t tab_round_up(size_t start) {
    return ((start / TAB_WIDTH) + 1) * TAB_WIDTH;
}

/*
 * Position on screen in the line for a position in the string.
 * Always left aligns tabs.
 *
 * buf: the line
 * ptr: a position in the line. line_pos_ptr(buf, buf) == 0.
 */
size_t line_pos_ptr(const char* buf, const char* ptr) {
    size_t pos_x = 0;
    for (; buf != ptr; ++buf) {
        if (*buf == BYTE_TAB) {
            pos_x = tab_round_up(pos_x);
        }
        else {
            ++pos_x;
        }
    }
    return pos_x;
}

/**
 * Return a pointer corresponding to the spot in buf at screen pos x. (0 indexed)
 */
char* line_pos(char* buf, ssize_t x) {
    ssize_t pos_x = 0;
    char* prev = NULL;
    for (; *buf; ++buf) {
        if (pos_x == x) {
            return buf;
        }
        else if (pos_x > x) {
            // Case of tab? If a tab jumps over, we return the tab.
            return prev;
        }
        if (*buf == BYTE_TAB) {
            // Tabbing behavior. Jump to the end of the tab.
            pos_x = tab_round_up(pos_x);
            if (pos_x > x) {
                // Case of tab? If a tab jumps over, we return the tab.
                return buf;
            }
        }
        else {
            ++pos_x;
        }
        prev = buf;
    }
    return buf;
}

/**
 * Get the character at the line position, or null.
 */
char line_pos_char(char* buf, size_t x) {
    char* pos_ptr = line_pos(buf, x);
    if (pos_ptr == NULL) {
        return 0;
    }
    return *pos_ptr;
}

/**
 * Compute the 'screen length' of a buffer. Accounts for tabs.
 */
size_t strlen_tab(const char* buf) {
    size_t ret = 0;
    for (; *buf; ++buf) {
        if (*buf == BYTE_TAB) {
            ret = tab_round_up(ret);
            continue;
        }
        if (*buf == BYTE_ENTER) {
            continue;
        }
        ++ret;
    }
    return ret;
}

/**
 * This function doesn't do much for now but will eventually handle line numbering / multiline.
 */
void format_left_bar(String** write_buffer, size_t i) {
    size_t line_idx = Buffer_get_line_index(current_buffer, i);
    char dest[editor_left+1];
    int n_print = snprintf(dest, editor_left+1, "%lu ", line_idx);
    if (n_print > 0) {
        size_t n = min_u(n_print, editor_left);
        memmove(dest + editor_left - n, dest, n);
        memset(dest, ' ', editor_left - n);
    }
    else {
        memset(dest, ' ', editor_left);
    }
    Strncats(write_buffer, dest, editor_left);
}

/**
 * Prepare a buffer to write out to the screen, respecting tab width.
 * Replaces tabs with spaces; Drops newlines.
 *
 * Parameters:
 *      write_buffer: String to push into to return.
 *      buf: data to write out. (C string, no control chars)
 *      start: Screenwidth position to start at.
 *      count: Number of characters in 'buf' to write out.
 * 
 * Return:
 *      New "effective screen position" (for tabbing purpoises)
 */
size_t format_respect_tabspace(String** write_buffer, const char* buf, size_t start, size_t count) {
    size_t zero_size = Strlen(*write_buffer);
    for (size_t i = 0; i < count; ++i) {
        if (buf[i] == BYTE_TAB) {
            size_t x = Strlen(*write_buffer) - zero_size + start;
            for (size_t j = x; j < tab_round_up(x); ++j) {
                String_push(write_buffer, ' ');
            }
            continue;
        }
        else if (buf[i] == BYTE_ENTER) {
            continue;
        }
        String_push(write_buffer, buf[i]);
    }
    return Strlen(*write_buffer) - zero_size + start;
}

/**
 * Write a buffer out to the screen, respecting tab width.
 * Replaces tabs with spaces.
 * 
 * Drops newlines.
 */
String* write_line_buffer = NULL;
size_t write_respect_tabspace(const char* buf, size_t start, size_t count) {
    String_clear(write_line_buffer);
    size_t ret = format_respect_tabspace(&write_line_buffer, buf, start, count);
    _write(write_line_buffer->data, Strlen(write_line_buffer));
    return ret;
}

/**
 * Creates a buffer for the given file and pushes it to the vector of buffers.
 */
void editor_make_buffer(const char* filename) {
    Buffer* buffer = make_Buffer(filename);
    Vector_push(&buffers, buffer);
}

void editor_switch_buffer(size_t n) {
    current_buffer_idx = n;
    current_buffer = buffers.elements[n];
    clear_screen();
}
    
/**
 * Performs initial setup for the editor, including making the vector of buffers,
 * creating and pushing a buffer for the given filename to that vector,
 * and configuring the size of the editor window. Defaults to `Normal` editing mode.
 */
void editor_init(const char* filename) {
    write_line_buffer = alloc_String(100);
    current_mode = EM_NORMAL;
    command_buffer = alloc_String(10);
    inplace_make_Vector(&buffers, 10);
    inplace_make_Vector(&active_copy.data, 10);
    current_buffer = make_Buffer(filename);
    Vector_push(&buffers, current_buffer);
    current_buffer_idx = 0;
    editor_top = 1;
    editor_left = 5;
    editor_display = 1;
    editor_macro_mode = 0;
    editor_window_size_change();
}

/**
 * Destroys the buffer at position `idx` in the vector of buffers and updates
 * the screen to display the previous buffer in the vector, if there is one.
 */
void editor_close_buffer(int idx) {
    Buffer* buf = buffers.elements[idx];
    Vector_delete(&buffers, idx);
    Buffer_destroy(buf);
    free(buf);
    if (idx == current_buffer_idx) {
        int new_idx;
        if (idx == 0) {
            new_idx = buffers.size - 1;
        }
        else {
            new_idx = current_buffer_idx - 1;
        }
        if (new_idx >= 0) {
            editor_switch_buffer(new_idx);
        }
    }
}

/**
 * Returns a pointer to the line at position y in the current buffer.
 */
char** get_line_in_buffer(size_t y) {
    return Buffer_get_line(current_buffer, y);
}

void begin_insert() {
    char* line = *get_line_in_buffer(current_buffer->cursor_row);
    char* head = line_pos(line, current_buffer->cursor_col);
    inplace_make_GapBuffer(&active_insert, line, DEFAULT_GAP_SIZE);
    gapBuffer_move_gap(&active_insert, head - line);
    // active_insert = make_Edit(current_buffer->undo_index, 
    //                     Buffer_get_line_index(current_buffer, current_buffer->cursor_row),
    //                     0, line);
    EditorMode mode = Buffer_get_mode(current_buffer);
    if (mode == EM_VISUAL || mode == EM_VISUAL_LINE) {
        Buffer_exit_visual(current_buffer);
    }
    Buffer_set_mode(current_buffer, EM_INSERT);
}

/**
 * ugly new method for now, as we transition away from Edit for current state.
 * For a delete action, pass in NULL.
 * Takes ownership of passed in pointer.
 */
void push_current_action(char* new_content) {
    size_t line_num = Buffer_get_line_index(current_buffer, current_buffer->cursor_row);
    char** line_p = Buffer_get_line_abs(current_buffer, line_num);
    // TODO: make constructor, check correctness
    Edit* action = malloc(sizeof(Edit));
    action->undo_index = current_buffer->undo_index;
    action->start_row = line_num;
    action->start_col = -1;
    action->old_content = *line_p;
    if (new_content == NULL) {
        Vector_delete(&current_buffer->lines, line_num);
        action->new_content = NULL;
    }
    else {
        *line_p = new_content;
        action->new_content = make_String(new_content);
    }
    Buffer_push_undo(current_buffer, action);
}

void end_insert() {
    // TODO: gapbuffer optimization
    char* content = gapBuffer_get_content(&active_insert);
    // consumes the content pointer. no need to free
    push_current_action(content);
    gapBuffer_destroy(&active_insert);
}

int editor_backspace() {
    int y_pos = current_buffer->cursor_row;
    size_t line_num = Buffer_get_line_index(current_buffer, y_pos);
    if (current_buffer->cursor_col == 0) {
        if (line_num > 0) {
            // Taking ownership of the buffer content.
            char* _content = active_insert.content;
            char* content = _content + active_insert.gap_end;
            size_t content_len = gapBuffer_content_length(&active_insert);
            push_current_action(NULL);

            inplace_make_GapBuffer(&active_insert, current_buffer->lines.elements[line_num-1], DEFAULT_GAP_SIZE);
            gapBuffer_move_gap(&active_insert, active_insert.total_size - active_insert.gap_end);
            gapBuffer_delete(&active_insert, 1);    // perform the actual delete

            current_buffer->cursor_col = strlen_tab(active_insert.content);
            current_buffer->natural_col = current_buffer->cursor_col;
            --current_buffer->cursor_row;
            move_to_current();
            write_respect_tabspace(content, current_buffer->cursor_col, content_len);
            gapBuffer_insertN(&active_insert, content, content_len);
            gapBuffer_move_gap(&active_insert, -content_len);

            free(_content);

            display_buffer_rows(y_pos, editor_bottom - editor_top);
            return 1;
        }
        return 0;
    }
    // NOTE: using content of gapbuffer directly since its the head!
    gapBuffer_delete(&active_insert, 1);    // perform the actual delete
    char* current_ptr = active_insert.content + active_insert.gap_end;
    // NOTE: strlen of gapbuffer head is ONLY safe immediately after a delete!
    current_buffer->cursor_col = strlen_tab(active_insert.content);
    current_buffer->natural_col = current_buffer->cursor_col;
    move_to_current();
    clear_line();
    // TODO: +1??
    write_respect_tabspace(current_ptr, current_buffer->cursor_col, strlen(current_ptr) + 1);
    return 1;
}

void add_chr(char c) {
    if (c == BYTE_ENTER) {
        char newline_c = '\n';
        gapBuffer_insertN(&active_insert, &newline_c, 1);
        char* content = active_insert.content; // taking ownership of buffer.
        char* rest = content + active_insert.gap_end;
        size_t rest_len = active_insert.total_size - active_insert.gap_end;
        // Save edited line, push to undo stack.
        // need to give it something to take ownership of.
        push_current_action(strndup(content, active_insert.gap_start));
        print("newline clear line\n");
        move_to_current();
        clear_line();

        // Make new line.
        editor_newline(1, content, rest);
        write_respect_tabspace(rest, 0, rest_len);
        free(content);

        print("newline fixview call\n");
        editor_fix_view();
        move_to_current();
        print("newline: col=%ld\n", current_buffer->cursor_col);
        return;
    }
    char* current_ptr = active_insert.content + active_insert.gap_start;
    size_t n_insert;
    if (c == BYTE_TAB && EXPAND_TAB) {
    	size_t fill_target = tab_round_up(current_buffer->cursor_col);
    	n_insert = fill_target - current_buffer->cursor_col;
    	char spaces[n_insert];
    	memset(spaces, ' ', n_insert);
        gapBuffer_insertN(&active_insert, spaces, n_insert);
    }
    else {
        n_insert = 1;
        gapBuffer_insertN(&active_insert, &c, n_insert);
    }
    String_clear(write_line_buffer);
    Strcats(&write_line_buffer, CLEAR_LINE);
    size_t pos = format_respect_tabspace(&write_line_buffer, current_ptr, current_buffer->cursor_col, n_insert);
    // add might realloc. no need to null terminate the string though
    current_ptr = active_insert.content + active_insert.gap_start;
    current_buffer->cursor_col = pos;
    current_buffer->natural_col = current_buffer->cursor_col;
    format_respect_tabspace(&write_line_buffer, active_insert.content + active_insert.gap_end, pos,
                            active_insert.total_size - active_insert.gap_end);
    _write(write_line_buffer->data, Strlen(write_line_buffer));
    move_to_current();
}

/**
 * "o" command in vim.
 * TODO: make "O".
 * Initial string doesn't need to be malloc'd or anything.
 */
void editor_newline(int side, const char* head, const char* initial) {
    if (side > 0) {
        int y_pos = current_buffer->cursor_row + 1;
        size_t line_num = Buffer_get_line_index(current_buffer, y_pos);
        // Copies 'initial' to the tail of this gapbuffer. Neat!
        inplace_make_GapBuffer(&active_insert, initial, DEFAULT_GAP_SIZE);
        size_t start_len = 0;
        if (PRESERVE_INDENT) {
            for (const char* ptr = head; is_whitespace(*ptr); ++ptr) {
                gapBuffer_insertN(&active_insert, ptr, 1);
            }
            start_len = strlen_tab(active_insert.content);
        }
        // HACK new action just to insert. TODO
        // lack of start info -- gapbuffer tracks a lot of nice metadata implicitly, but not the insert status.
        Vector_insert(&current_buffer->lines, line_num, strdup(""));
        Edit* newline_edit = make_Insert(current_buffer->undo_index, line_num, -1, "");
        Buffer_push_undo(current_buffer, newline_edit);
        current_buffer->cursor_row += 1;
        current_buffer->cursor_col = start_len;
        current_buffer->natural_col = start_len;
        display_buffer_rows(y_pos, editor_bottom - editor_top);
    }
}

void display_bottom_bar(char* left, char* right) {
    if (SCREEN_WRITE && editor_display) {
        size_t x, y;
        get_cursor_pos(&y, &x);
        move_cursor(window_size.ws_row-1, 0);
        _write(left, strlen(left));
        _write("\0", 1);
        clear_line();
        if (right != NULL) {
            size_t off = strlen(right) + 1;
            move_cursor(window_size.ws_row-1, window_size.ws_col - off);
            _write(right, strlen(right));
        }
        print("Displayed bottom bar [%s]\n", left);
        move_cursor(y, x);
    }
}

void format_line_highlight(String** buf, const char* line) {
    Strcats(buf, SET_HIGHLIGHT);
    Strcats(buf, line);
    Strcats(buf, RESET_HIGHLIGHT);
}

void display_line_highlight(const char* line) {
    static_String(buf, 10);
    format_line_highlight(&buf, line);
    write_respect_tabspace(buf->data, 0, Strlen(buf));
}

/**
 * Display rows [start, end] inclusive, in screen coords (1-indexed).
 * Return value is for debugging.
 */
char* display_buffer_rows(ssize_t start, ssize_t end) {
    if (start < 0) { start = 0; }
    if (end > (editor_bottom - editor_top)) { end = editor_bottom - editor_top; }
    if (!editor_display) {
        return "";
    }
    print("display: %lu %lu\n", start, end);
    move_cursor(start + editor_top, 0);
    static_String(output_buffer, 100);
    bool highlight_mode = false;

    //TODO handle line overruns
    size_t cur_idx = Buffer_get_line_index(current_buffer, current_buffer->cursor_row);
    print("Buffer: %ld\n", current_buffer->cursor_row);
    EditorContext visual_bounds = { .start_row = -1, .jump_row = -1 };
    EditorMode mode = Buffer_get_mode(current_buffer);
    if (mode == EM_VISUAL || mode == EM_VISUAL_LINE) {
        // NOTE: EDITOR MUST NOT BE IN INSERT MODE!
        size_t cur_col = -1;
        if (mode == EM_VISUAL) {
            char* cur_line = *Buffer_get_line_abs(current_buffer, cur_idx);
            cur_col = line_pos(cur_line, current_buffer->cursor_col) - cur_line;
        }
        normalize_context(&visual_bounds,
                          current_buffer->visual_row, current_buffer->visual_col,
                          cur_idx, cur_col);
        size_t start_line = Buffer_get_line_index(current_buffer, start);
        if (start_line > visual_bounds.start_row && start_line <= visual_bounds.jump_row) {
            highlight_mode = true;
        }
    }

    for (size_t i = start; i < end; ++i) {
        if (highlight_mode) { Strcats(&output_buffer, RESET_HIGHLIGHT); }
        Strcats(&output_buffer, CLEAR_LINE);
        if (highlight_mode) { Strcats(&output_buffer, SET_HIGHLIGHT); }

        size_t line_idx = Buffer_get_line_index(current_buffer, i);
        if (line_idx < current_buffer->lines.size) {
            char* str;
            if (active_insert.content != NULL && current_buffer->cursor_row == i) {
                format_left_bar(&output_buffer, i);
                str = gapBuffer_get_content(&active_insert);
                format_respect_tabspace(&output_buffer, str, 0, strlen(str));
                free(str);
            }
            else {
                str = *Buffer_get_line_abs(current_buffer, line_idx);
                if (line_idx == visual_bounds.start_row) {
                    if (visual_bounds.start_col == -1) {
                        Strcats(&output_buffer, SET_HIGHLIGHT);
                        format_left_bar(&output_buffer, i);
                        format_respect_tabspace(&output_buffer, str, 0, strlen(str));
                        if (line_idx == visual_bounds.jump_row) {
                            Strcats(&output_buffer, RESET_HIGHLIGHT);
                        }
                        else {
                            highlight_mode = true;
                        }
                    }
                    else {
                        format_left_bar(&output_buffer, i);
                        size_t pos = format_respect_tabspace(&output_buffer,
                                                    str, 0, visual_bounds.start_col);
                        Strcats(&output_buffer, SET_HIGHLIGHT);
                        if (line_idx == visual_bounds.jump_row) {
                            pos = format_respect_tabspace(&output_buffer,
                                        str + visual_bounds.start_col, pos, 
                                        visual_bounds.jump_col - visual_bounds.start_col + 1);
                            Strcats(&output_buffer, RESET_HIGHLIGHT);
                            format_respect_tabspace(&output_buffer,
                                        str + visual_bounds.jump_col + 1, pos, 
                                        strlen(str + visual_bounds.jump_col + 1));
                        }
                        else {
                            format_respect_tabspace(&output_buffer,
                                        str + visual_bounds.start_col, pos, 
                                        strlen(str + visual_bounds.start_col));
                            highlight_mode = true;
                        }
                    }
                }
                else if (line_idx == visual_bounds.jump_row) {
                    if (visual_bounds.jump_col == -1) {
                        format_left_bar(&output_buffer, i);
                        format_respect_tabspace(&output_buffer, str, 0, strlen(str));
                        Strcats(&output_buffer, RESET_HIGHLIGHT);
                        highlight_mode = false;
                    }
                    else {
                        format_left_bar(&output_buffer, i);
                        size_t pos = format_respect_tabspace(&output_buffer,
                                                str, 0, visual_bounds.jump_col + 1);
                        Strcats(&output_buffer, RESET_HIGHLIGHT);
                        format_respect_tabspace(&output_buffer,
                                                str + visual_bounds.jump_col + 1, pos, 
                                                strlen(str + visual_bounds.jump_col + 1));
                    }
                }
                else {
                    format_left_bar(&output_buffer, i);
                    format_respect_tabspace(&output_buffer, str, 0, strlen(str));
                }
            }
        }
        String_push(&output_buffer, '\n');
    }
    Strcats(&output_buffer, RESET_HIGHLIGHT);
    _write(output_buffer->data, Strlen(output_buffer));
    move_to_current();
    return output_buffer->data;
}

void display_current_buffer() {
    display_buffer_rows(0, editor_bottom-editor_top);
}

void left_align_tab(char* line) {
    if (line_pos_char(line, current_buffer->cursor_col) == BYTE_TAB) {
        do {
            --current_buffer->cursor_col;
        } while (current_buffer->cursor_col % TAB_WIDTH != 3
                    && line_pos_char(line, current_buffer->cursor_col) == BYTE_TAB);
        ++current_buffer->cursor_col;
    }
}

void right_align_tab(char* line) {
    if (line_pos_char(line, current_buffer->cursor_col) == BYTE_TAB) {
        do {
            ++current_buffer->cursor_col;
        } while (current_buffer->cursor_col % TAB_WIDTH
                    && line_pos_char(line, current_buffer->cursor_col) == BYTE_TAB);
        --current_buffer->cursor_col;
    }
}

void editor_move_up() {
    bool insert = false;
    if (active_insert.content != NULL) {
        insert = true;
        end_insert();
    }
    current_buffer->cursor_row -= 1;
    editor_fix_view();
    char* line = *get_line_in_buffer(current_buffer->cursor_row);
    size_t col = strlen_tab(line);
    if (!insert && col > 0) --col;
    if (col > current_buffer->natural_col) { col = current_buffer->natural_col; }
    current_buffer->cursor_col = col;
    editor_align_tab();
    if (insert) {
        begin_insert();
    }
}
void editor_move_down() {
    bool insert = false;
    if (active_insert.content != NULL) {
        insert = true;
        end_insert();
    }
    current_buffer->cursor_row += 1;
    editor_fix_view();
    char* line = *get_line_in_buffer(current_buffer->cursor_row);
    size_t col = strlen_tab(line);
    if (!insert && col > 0) --col;
    if (col > current_buffer->natural_col) { col = current_buffer->natural_col; }
    current_buffer->cursor_col = col;
    editor_align_tab();
    if (insert) {
        begin_insert();
    }
}

void editor_move_EOL() {
    char* line = *get_line_in_buffer(current_buffer->cursor_row);
    int max_char = strlen(line) - 1;
    // Save newlines, but don't count them towards line length for cursor purposes.
    if (strlen(line) != 0 && line[strlen(line) - 1] == '\n') {
        max_char -= 1;
    }
    while (line_pos(line, current_buffer->cursor_col) - line < max_char) {
        if (active_insert.content != NULL) {
            if (line_pos_char(line, current_buffer->cursor_col) == BYTE_TAB) {
                right_align_tab(line);
            }
            ++current_buffer->cursor_col;
        }
        else {
            ++current_buffer->cursor_col;
            right_align_tab(line);
        }
    }
    current_buffer->natural_col = current_buffer->cursor_col;
}

void editor_move_left() {
    char* line;
    bool insert = false;
    if (active_insert.content != NULL) {
        insert = true;
        line = gapBuffer_get_content(&active_insert);
    }
    else {
        line = *get_line_in_buffer(current_buffer->cursor_row);
    }
    if (line_pos(line, current_buffer->cursor_col) != line) {
        if (insert) {
            gapBuffer_move_gap(&active_insert, -1);
            if (line_pos_char(line, current_buffer->cursor_col) == BYTE_TAB) {
                left_align_tab(line);
            }
            --current_buffer->cursor_col;
        }
        else {
            --current_buffer->cursor_col;
            left_align_tab(line);
        }
    }
    if (insert) {
        free(line);
    }
    current_buffer->natural_col = current_buffer->cursor_col;
}
void editor_move_right() {
    int max_char = -1;
    char* line;
    bool insert = false;
    if (active_insert.content != NULL) {
        insert = true;
        line = gapBuffer_get_content(&active_insert);
        max_char = 0;
    }
    else {
        line = *get_line_in_buffer(current_buffer->cursor_row);
    }
    max_char += strlen(line);
    // Save newlines, but don't count them towards line length for cursor purposes.
    if (strlen(line) != 0 && line[strlen(line) - 1] == '\n') {
        max_char -= 1;
    }
    if (line_pos(line, current_buffer->cursor_col) - line < max_char) {
        if (insert) {
            gapBuffer_move_gap(&active_insert, 1);
            if (line_pos_char(line, current_buffer->cursor_col) == BYTE_TAB) {
                right_align_tab(line);
            }
            ++current_buffer->cursor_col;
        }
        else {
            ++current_buffer->cursor_col;
            right_align_tab(line);
        }
    }
    if (insert) {
        free(line);
    }
    current_buffer->natural_col = current_buffer->cursor_col;
}

RepaintType editor_fix_view() {
    //TODO more than 1
    ssize_t bottom_limit = ((ssize_t) editor_bottom) - (editor_top + 1);
    ssize_t buffer_limit = (ssize_t) current_buffer->lines.size - 1;
    bool display = false;
    if (buffer_limit < bottom_limit) bottom_limit = buffer_limit;
    if (current_buffer->cursor_row > bottom_limit) {
        ssize_t delta = current_buffer->cursor_row - bottom_limit;
        current_buffer->cursor_row -= delta;
        Buffer_scroll(current_buffer, editor_bottom-editor_top, delta);
        display = true;
    }
    if (current_buffer->cursor_row < 0) {
        ssize_t delta = -current_buffer->cursor_row;
        current_buffer->cursor_row += delta;
        Buffer_scroll(current_buffer, editor_bottom-editor_top, -delta);
        display = true;
    }
    char** line_p = get_line_in_buffer(current_buffer->cursor_row);
    while (line_p == NULL) {
        --current_buffer->cursor_row;
        line_p = get_line_in_buffer(current_buffer->cursor_row);
    }
    char* line;
    size_t max_col;
    if (active_insert.content != NULL) {
        line = active_insert.content;   // lol jankish
        max_col = strlen_tab(line)+1;
    }
    else {
        line = *line_p;
        max_col = strlen_tab(line);
    }
    if (current_buffer->cursor_col >= max_col) {
        if (max_col == 0) {
            current_buffer->cursor_col = 0;
        }
        else {
            current_buffer->cursor_col = max_col - 1;
        }
    }
    if (display) {
        print("Fix view repaint\n");
        display_current_buffer();
        return RP_ALL;
    }
    return RP_NONE;
}

void editor_align_tab() {
    if (active_insert.content != NULL) {
        // Left align in insert mode.
        char* line = gapBuffer_get_content(&active_insert);
        left_align_tab(line);
        free(line);
    }
    else {
        // Right align in normal mode.
        right_align_tab(*get_line_in_buffer(current_buffer->cursor_row));
    }
}

/**
 * Move to arbitrary (row, col). Clips to the editor's window.
 */
RepaintType editor_move_to(ssize_t row, ssize_t col) {
    if (row < 0) row = 0;
    if (col < 0) col = 0;
    ssize_t current_row = Buffer_get_line_index(current_buffer, current_buffer->cursor_row);
    ssize_t delta = row - current_row;
    current_buffer->cursor_row += delta;
    current_buffer->cursor_col = 0;
    RepaintType ret = editor_fix_view();
    char* line = *get_line_in_buffer(current_buffer->cursor_row);
    if (col > strlen(line)) col = strlen(line);
    col = line_pos_ptr(line, line+col);
    size_t max_col = strlen_tab(line);
    if (max_col > 0) --max_col;
    if (col > max_col) col = max_col;
    current_buffer->cursor_col = col;
    right_align_tab(line);
    if (delta == 0) {
        current_buffer->natural_col = current_buffer->cursor_col;
    }
    move_to_current();
    return ret;
}

/**
 * Repaints part of the editor, depending on the action type
 * and position information contained in `ctx`.
 * NOTE: MUST TAKE A NORMALIZED CONTEXT!
 */
void editor_repaint(RepaintType repaint, EditorContext* ctx) {
    Buffer* buf = ctx->buffer;
    editor_fix_view();
    move_to_current();
    if (repaint == RP_ALL) {
        display_current_buffer();
    }
    else if (repaint == RP_LINES) {
        if (ctx->start_row <= ctx->jump_row) {
            display_buffer_rows(ctx->start_row - buf->top_row,
                                1 + ctx->jump_row - buf->top_row);
        }
        else {
            display_buffer_rows(ctx->jump_row - buf->top_row,
                                1 + ctx->start_row - buf->top_row);
        }
    }
    else if (repaint == RP_LOWER) {
        print("repaint lower\n");
        display_buffer_rows(ctx->start_row - buf->top_row, editor_bottom - editor_top);
    }
    else if (repaint == RP_UPPER) {
        print("repaint upper\n");
        display_buffer_rows(0, 1 + ctx->start_row - buf->top_row);
    }
}
