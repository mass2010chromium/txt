#include <stdlib.h>
#include <termios.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "editor.h"
#include "utils.h"
#include "debugging.h"

const char BYTE_ESC         = '\033';
//const char BYTE_ENTER       = '\015';     // Carriage Return
const char BYTE_TAB         = '\011';     // Tab
const char BYTE_ENTER       = '\012';     // Line Feed
const char BYTE_BACKSPACE   = '\177';
const char BYTE_UPARROW     = '\110';
const char BYTE_LEFTARROW   = '\113';
const char BYTE_RIGHTARROW  = '\115';
const char BYTE_DOWNARROW   = '\120';
const char BYTE_DELETE      = '\123';

int TAB_WIDTH = 4;
bool PRESERVE_INDENT = true;

Copy active_copy = {0};

struct winsize window_size = {0};

Buffer* current_buffer = NULL;

Vector/*Buffer* */ buffers = {0};
String* command_buffer = NULL;
GapBuffer active_insert = {0};
size_t editor_top;
size_t editor_bottom;

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
    current_buffer->undo_index += 1;
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

/**
 * Clear the current line of text.
 * I'm doing this by writing the empty space char instead of using
 * control chars in an attempt to make it work on tmux wsl 20.04.
 */
void clear_line() {
//     size_t x, y;
//     get_cursor_pos(&y, &x);
//     int n = window_size.ws_col - x;
//     char* buf = malloc(n + 1);
//     buf[n] = 0;
//     memset(buf, ' ', n);
//     write(STDOUT_FILENO, buf, n);
//     free(buf);
//     move_cursor(y, x);
    write(STDOUT_FILENO, "\033[0K", 4);
}

/**
 * Clear the entire screen by writing a control character.
 */
void clear_screen() {
    write(STDOUT_FILENO, "\033[2J", 4);
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
   
    write(STDOUT_FILENO, "\033[6n", 4);
   
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
    char buf[60] = {0};
    sprintf(buf, "\033[%lu;%luH", y+1, x+1);
    write(STDOUT_FILENO, buf, strlen(buf));
}

/**
 * Move the cursor to the current (row, col) of the buffer.
 */
void move_to_current() {
    move_cursor(current_buffer->cursor_row, current_buffer->cursor_col);
}

/*
 * Position on screen in the line for a position in the string.
 * buf: the line
 * ptr: a position in the line. line_pos_ptr(buf, buf) == 0.
 */
size_t line_pos_ptr(const char* buf, const char* ptr) {
    size_t pos_x = 0;
    for (; buf != ptr; ++buf) {
        if (*buf == BYTE_TAB) {
            pos_x = ((pos_x / TAB_WIDTH)+1) * TAB_WIDTH;
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
            pos_x = ((pos_x / TAB_WIDTH)+1) * TAB_WIDTH;
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
 * Get the character corresponding to sceen pos (y, x). Zero indexed.
 */
char screen_pos_char(size_t y, size_t x) {
    return line_pos_char(*get_line_in_buffer(y), x);
}

/**
 * convenience function for getting current screen pos.
 */
char current_screen_pos_char() {
    return screen_pos_char(current_buffer->cursor_row, current_buffer->cursor_col);
}

/**
 * Compute the 'screen length' of a buffer. Accounts for tabs.
 */
size_t strlen_tab(const char* buf) {
    size_t ret = 0;
    for (; *buf; ++buf) {
        if (*buf == BYTE_TAB) {
            ret = ((ret / TAB_WIDTH) + 1) * TAB_WIDTH;
            continue;
        }
        ++ret;
    }
    return ret;
}

size_t format_respect_tabspace(String** write_buffer, const char* buf, size_t start, size_t count) {
    for (size_t i = 0; i < count; ++i) {
        if (buf[i] == BYTE_TAB) {
            size_t x = Strlen(*write_buffer) + start;
            for (int j = x; j < ((x / TAB_WIDTH) + 1) * TAB_WIDTH; ++j) {
                String_push(write_buffer, ' ');
            }
            continue;
        }
        String_push(write_buffer, buf[i]);
    }
    return Strlen(*write_buffer) + start;
}

/**
 * Write a buffer out to the screen, respecting tab width.
 * Replaces tabs with spaces.
 */
size_t write_respect_tabspace(const char* buf, size_t start, size_t count) {
    static String* write_buffer = NULL;
    if (write_buffer == NULL) write_buffer = alloc_String(count);
    else String_clear(write_buffer);
    size_t ret = format_respect_tabspace(&write_buffer, buf, start, count);
    write(STDOUT_FILENO, write_buffer->data, Strlen(write_buffer));
    return ret;
}

/**
 * Creates a buffer for the given file and pushes it to the vector of buffers.
 */
void editor_make_buffer(const char* filename) {
    Buffer* buffer = make_Buffer(filename);
    Vector_push(&buffers, buffer);
}
    
/**
 * Performs initial setup for the editor, including making the vector of buffers,
 * creating and pushing a buffer for the given filename to that vector,
 * and configuring the size of the editor window. Defaults to `Normal` editing mode.
 */
void editor_init(const char* filename) {
    current_mode = EM_NORMAL;
    command_buffer = alloc_String(10);
    inplace_make_Vector(&buffers, 10);
    inplace_make_Vector(&active_copy.data, 10);
    current_buffer = make_Buffer(filename);
    Vector_push(&buffers, current_buffer);
    current_buffer_idx = 0;
    editor_top = 1; // TODO this setting is broken
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
        if (idx == 0) {
            current_buffer_idx = buffers.size-1;
        }
        else {
            --current_buffer_idx;
        }
        if (current_buffer_idx >= 0) {
            current_buffer = buffers.elements[current_buffer_idx];
        }
    }
}

/**
 * Returns a pointer to the line at position y in the current buffer.
 */
char** get_line_in_buffer(size_t y) {
    return Buffer_get_line(current_buffer, y);
}

//TODO rewrite this?
size_t screen_pos_to_file_pos(size_t y, size_t x, size_t* line_start) {
    size_t start = current_buffer->top_left_file_pos;
    size_t end = y-1;
    if (end > current_buffer->lines.size) {
        end = current_buffer->lines.size - current_buffer->top_row;
    }
    for (size_t r = 0; r < end; ++r) {
        start += strlen(*get_line_in_buffer(r));
    }
    *line_start = start;
    start += (x - 1);
    return start;
}

void begin_insert() {
    char* line = *get_line_in_buffer(current_buffer->cursor_row);
    char* head = line_pos(line, current_buffer->cursor_col);
    inplace_make_GapBuffer(&active_insert, line, DEFAULT_GAP_SIZE);
    gapBuffer_move_gap(&active_insert, head - line);
    // active_insert = make_Edit(current_buffer->undo_index, 
    //                     Buffer_get_line_index(current_buffer, current_buffer->cursor_row),
    //                     0, line);
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
    action->start_row = current_buffer->cursor_row;
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

/**
 * Delete a character under the cursor (except newlines).
 * DEPRECATED, kept for future reference for now
 */
int del_chr() {
    size_t y_pos = current_buffer->cursor_row;
    size_t x_pos = current_buffer->cursor_col;
    char** lineptr = (char**) get_line_in_buffer(y_pos);
    char* line = *lineptr;
    if (strlen(line) == 0) {
        return 0;
    }
    else if (strlen(line) == 1 && line[0] == '\n') {
        return 0;
    }
    Edit* delete_action = make_Edit(current_buffer->undo_index, 
                        Buffer_get_line_index(current_buffer, y_pos), -1, line);
    char* current_ptr = line_pos(delete_action->new_content->data, x_pos);
    size_t rest = Strlen(delete_action->new_content)
                    - (current_ptr - delete_action->new_content->data);
    char buf[2] = {0, 0};
    buf[0] = String_delete(delete_action->new_content, current_ptr - delete_action->new_content->data);
    clear_line();
    write_respect_tabspace(current_ptr, x_pos, rest);
    free(line);
    *lineptr = strdup(delete_action->new_content->data);
    Buffer_push_undo(current_buffer, delete_action);
    char hover_ch = line_pos_char(*lineptr, current_buffer->cursor_col);
    for (int i = 0; i < TAB_WIDTH && (hover_ch == 0 || hover_ch == '\n'); ++i) {
        editor_move_left();
        hover_ch = line_pos_char(*lineptr, current_buffer->cursor_col);
    }
    return 1;
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

            display_buffer_rows(y_pos+1, editor_bottom);
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
        push_current_action(strndup(content, active_insert.gap_start));   // need to give it something to take ownership of.
        move_to_current();
        clear_line();

        // Make new line.
        editor_newline(1, content, rest);
        write_respect_tabspace(rest, 0, rest_len);
        free(content);

        editor_fix_view();
        move_to_current();
        return;
    }
    char* current_ptr = active_insert.content + active_insert.gap_start;
    gapBuffer_insertN(&active_insert, &c, 1);
    clear_line();
    write_respect_tabspace(current_ptr, current_buffer->cursor_col, 1);
    // add might realloc. no need to null terminate the string though
    current_ptr = active_insert.content + active_insert.gap_start;
    current_buffer->cursor_col = line_pos_ptr(active_insert.content, current_ptr);
    current_buffer->natural_col = current_buffer->cursor_col;
    write_respect_tabspace(active_insert.content + active_insert.gap_end, current_buffer->cursor_col,
                            active_insert.total_size - active_insert.gap_end);
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
        inplace_make_GapBuffer(&active_insert, initial, DEFAULT_GAP_SIZE);  // Copies 'initial' to the tail of this gapbuffer. Neat!
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
        current_buffer->cursor_col = 0;
        current_buffer->cursor_col = start_len;
        current_buffer->natural_col = start_len;
        display_buffer_rows(y_pos+1, editor_bottom);
    }
}

void display_bottom_bar(char* left, char* right) {
    size_t x, y;
    get_cursor_pos(&y, &x);
    move_cursor(window_size.ws_row-1, 0);
    write(STDOUT_FILENO, left, strlen(left));
    write(STDOUT_FILENO, "\0", 1);
    clear_line();
    if (right != NULL) {
        size_t off = strlen(right);
        move_cursor(window_size.ws_row, window_size.ws_col - off);
        write(STDOUT_FILENO, right, strlen(right));
    }
    print("Displayed bottom bar [%s]\n", left);
    move_cursor(y, x);
}

void format_line_highlight(String** buf, const char* line) {
    Strcats(buf, SET_HIGHLIGHT);
    Strcats(buf, line);
    Strcats(buf, RESET_HIGHLIGHT);
}

void display_line_highlight(const char* line) {
    static String* buf = NULL;
    if (buf == NULL) { buf = alloc_String(10); }
    else { String_clear(buf); }
    format_line_highlight(&buf, line);
    write_respect_tabspace(buf->data, 0, Strlen(buf));
}

/**
 * Display rows [start, end] inclusive, in screen coords (1-indexed).
 */
void display_buffer_rows(size_t start, size_t end) {
    //TODO handle line overruns
    //TODO only redraw changes
    size_t cur_idx = Buffer_get_line_index(current_buffer, current_buffer->cursor_row);
    ssize_t visual_start_row = -1;
    ssize_t visual_end_row = -1;
    ssize_t visual_start_col = -1;
    ssize_t visual_end_col = -1;
    EditorMode mode = Buffer_get_mode(current_buffer);
    if (mode == EM_VISUAL || mode == EM_VISUAL_LINE) {
        visual_start_row = min_u(cur_idx, current_buffer->visual_row);
        visual_end_row = max_u(cur_idx, current_buffer->visual_row);
        if (mode == EM_VISUAL) {
            // NOTE: EDITOR MUST NOT BE IN INSERT MODE!
            char* cur_line = *Buffer_get_line_abs(current_buffer, cur_idx);
            size_t cur_col = line_pos(cur_line, current_buffer->cursor_col) - cur_line;
            if (cur_idx == current_buffer->visual_row) {
                visual_start_col = min_u(cur_col, current_buffer->visual_col);
                visual_end_col = max_u(cur_col, current_buffer->visual_col);
            }
            else if (cur_idx < current_buffer->visual_row) {
                visual_start_col = cur_col;
                visual_end_col = current_buffer->visual_col;
            }
            else {//if (cur_idx > current_buffer->visual_row) {
                visual_start_col = current_buffer->visual_col;
                visual_end_col = cur_col;
            }
        }
        size_t start_idx = Buffer_get_line_index(current_buffer, start-1);
        if (start_idx > visual_start_row && start_idx <= visual_end_row) {
            write(STDOUT_FILENO, SET_HIGHLIGHT, strlen(SET_HIGHLIGHT));
        }
        fprintf(stderr, "visual (%lu, %lu) (%lu, %lu)\n",
                    visual_start_row,
                    visual_start_col,
                    visual_end_row,
                    visual_end_col);
    }
    for (size_t i = start; i <= end; ++i) {
        move_cursor(i-1, 0);
        clear_line();
        size_t line_idx = Buffer_get_line_index(current_buffer, i-1);
        if (line_idx < current_buffer->lines.size) {
            char* str;
            if (active_insert.content != NULL && current_buffer->cursor_row == i-1) {
                str = gapBuffer_get_content(&active_insert);
                if (strlen(str) != 0) {
                    write_respect_tabspace(str, 0, strlen(str));
                }
                free(str);
            }
            else {
                str = *Buffer_get_line_abs(current_buffer, line_idx);
                if (line_idx == visual_start_row) {
                    if (visual_start_col == -1) {
                        // TODO: Fix this to be more performant (less write calls) using buffer
                        // and format_respect_tabspace()
                        write(STDOUT_FILENO, SET_HIGHLIGHT, strlen(SET_HIGHLIGHT));
                        write(STDOUT_FILENO, str, strlen(str));
                    }
                    else {
                        size_t pos = write_respect_tabspace(str, 0, visual_start_col);
                        write(STDOUT_FILENO, SET_HIGHLIGHT, strlen(SET_HIGHLIGHT));
                        if (line_idx == visual_end_row) {
                            pos = write_respect_tabspace(str + visual_start_col, pos, 
                                                visual_end_col - visual_start_col + 1);
                            write(STDOUT_FILENO, RESET_HIGHLIGHT, strlen(RESET_HIGHLIGHT));
                            write_respect_tabspace(str + visual_end_col + 1, pos, 
                                                strlen(str + visual_end_col + 1));
                        }
                        else {
                            write_respect_tabspace(str + visual_start_col, pos, 
                                                    strlen(str + visual_start_col));
                        }
                    }
                }
                else if (line_idx == visual_end_row) {
                    if (visual_end_col == -1) {
                        write(STDOUT_FILENO, str, strlen(str));
                        write(STDOUT_FILENO, RESET_HIGHLIGHT, strlen(RESET_HIGHLIGHT));
                    }
                    else {
                        size_t pos = write_respect_tabspace(str, 0,
                                                visual_end_col + 1);
                        write(STDOUT_FILENO, RESET_HIGHLIGHT, strlen(RESET_HIGHLIGHT));
                        write_respect_tabspace(str + visual_end_col + 1, pos, 
                                                strlen(str + visual_end_col + 1));
                    }
                }
                else if (strlen(str) != 0) {
                    write_respect_tabspace(str, 0, strlen(str));
                }
            }

        }
    }
    write(STDOUT_FILENO, RESET_HIGHLIGHT, strlen(RESET_HIGHLIGHT));
    move_to_current();
}

void display_current_buffer() {
    display_buffer_rows(editor_top, editor_bottom);
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
    if (strlen(line) != 0 && line[strlen(line) - 1] == '\n') --col;
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
    if (strlen(line) != 0 && line[strlen(line) - 1] == '\n') --col;
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

void editor_fix_view() {
    //TODO more than 1
    ssize_t bottom_limit = ((ssize_t) editor_bottom) - 1;
    ssize_t buffer_limit = (ssize_t) current_buffer->lines.size - 1;
    bool display = false;
    if (buffer_limit < bottom_limit) bottom_limit = buffer_limit;
    if (current_buffer->cursor_row > bottom_limit) {
        ssize_t delta = current_buffer->cursor_row - bottom_limit;
        current_buffer->cursor_row -= delta;
        Buffer_scroll(current_buffer, editor_bottom+1-editor_top, delta);
        display = true;
    }
    if (current_buffer->cursor_row < (ssize_t) (editor_top-1)) {
        ssize_t delta = (ssize_t) (editor_top-1) - current_buffer->cursor_row;
        current_buffer->cursor_row += delta;
        Buffer_scroll(current_buffer, editor_bottom+1-editor_top, -delta);
        display = true;
    }
    char** line_p = get_line_in_buffer(current_buffer->cursor_row);
    while (line_p == NULL) {
        --current_buffer->cursor_row;
        line_p = get_line_in_buffer(current_buffer->cursor_row);
    }
    size_t max_col = strlen_tab(*line_p);
    if (current_buffer->cursor_col >= max_col) {
        if (max_col == 0) {
            current_buffer->cursor_col = 0;
        }
        else {
            current_buffer->cursor_col = max_col - 1;
        }
    }
    if (display) {
        display_current_buffer();
    }

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

void editor_move_to(ssize_t row, ssize_t col) {
    if (row < 0) row = 0;
    if (col < 0) col = 0;
    ssize_t current_row = Buffer_get_line_index(current_buffer, current_buffer->cursor_row);
    ssize_t delta = ((ssize_t) row) - current_row;
    current_buffer->cursor_row += delta;
    current_buffer->cursor_col = 0;
    editor_fix_view();
    char* line = *get_line_in_buffer(current_buffer->cursor_row);
    if (col > strlen(line)) col = strlen(line);
    col = line_pos_ptr(line, line+col);
    size_t max_col = strlen_tab(line);
    if (strlen(line) != 0 && line[strlen(line) - 1] == '\n') --max_col;
    if (max_col > 0) --max_col;
    if (col > max_col) col = max_col;
    current_buffer->cursor_col = col;
    right_align_tab(line);
    if (delta == 0) {
        current_buffer->natural_col = current_buffer->cursor_col;
    }
    move_to_current();
}

void editor_repaint(RepaintType repaint, EditorContext* ctx) {
    Buffer* buf = ctx->buffer;
    move_to_current();
    editor_fix_view();
    if (repaint == RP_ALL) {
        display_current_buffer();
    }
    else if (repaint == RP_LINES) {
        if (ctx->start_row <= ctx->jump_row) {
            display_buffer_rows(editor_top + ctx->start_row - buf->top_row,
                                editor_top + ctx->jump_row - buf->top_row);
        }
        else {
            display_buffer_rows(editor_top + ctx->jump_row - buf->top_row,
                                editor_top + ctx->start_row - buf->top_row);
        }
    }
    else if (repaint == RP_LOWER) {
        display_buffer_rows(editor_top + ctx->start_row - buf->top_row, window_size.ws_row - editor_top);
    }
    else if (repaint == RP_UPPER) {
        display_buffer_rows(editor_top, editor_top + ctx->start_row - buf->top_row);
    }
}
