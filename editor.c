#include <stdlib.h>
#include <termios.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "editor.h"
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

typedef int EditorMode;
const EditorMode EM_QUIT    = -1;
const EditorMode EM_NORMAL  = 0;
const EditorMode EM_INSERT  = 1;
const EditorMode EM_COMMAND = 2;

Copy active_copy = {0};

struct winsize window_size = {0};

Buffer* current_buffer = NULL;

Vector/*Buffer* */ buffers = {0};
String* command_buffer = NULL;
Edit* active_insert = NULL;
size_t editor_top;
size_t editor_bottom;

void editor_window_size_change() {
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &window_size);
    editor_bottom = window_size.ws_row - 1;
}

void editor_new_action() {
    current_buffer->undo_index += 1;
}

int editor_undo() {
    int num_undo = Buffer_undo(current_buffer, current_buffer->undo_index);
    if (num_undo > 0) {
        current_buffer->undo_index -= 1;
        display_current_buffer();
    }
    return num_undo;
}

void clear_line() {
    size_t x, y;
    get_cursor_pos(&y, &x);
    int n = window_size.ws_col - x;
    char* buf = malloc(n + 1);
    buf[n] = 0;
    memset(buf, ' ', n);
    write(STDOUT_FILENO, buf, n);
    free(buf);
    move_cursor(y, x);
    //write(STDOUT_FILENO, "\033[0K", 4);
}

void clear_screen() {
    write(STDOUT_FILENO, "\033[2J", 4);
}

/**
 * Adapted from https://stackoverflow.com/questions/50884685/how-to-get-cursor-position-in-c-using-ansi-code
 * See: https://en.wikipedia.org/wiki/ANSI_escape_code (Device Status Report)
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
 * Zero indexed!
 */
void move_cursor(size_t y, size_t x) {
    char buf[60] = {0};
    sprintf(buf, "\033[%lu;%luH", y+1, x+1);
    write(STDOUT_FILENO, buf, strlen(buf));
}

void move_to_current() {
    move_cursor(current_buffer->cursor_row, current_buffer->cursor_col);
}

/*
 * Position on screen in the line for a position in the string.
 * buf: the line
 * ptr: a position in the line
 */
size_t line_pos_ptr(char* buf, char* ptr) {
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
 * Pointer corresponding to the spot in buf at screen pos x. (0 indexed)
 */
char* line_pos(char* buf, ssize_t x) {
    ssize_t pos_x = 0;
    char* prev = NULL;
    for (; *buf; ++buf) {
        if (pos_x == x) {
            return buf;
        }
        else if (pos_x > x) {
            // Case of tab?
            return prev;
        }
        if (*buf == BYTE_TAB) {
            pos_x = ((pos_x / TAB_WIDTH)+1) * TAB_WIDTH;
        }
        else {
            ++pos_x;
        }
        prev = buf;
    }
    return buf;
}

char line_pos_char(char* buf, size_t x) {
    char* pos_ptr = line_pos(buf, x);
    if (pos_ptr == NULL) {
        return 0;
    }
    return *pos_ptr;
}

char screen_pos_char(size_t y, size_t x) {
    return line_pos_char(*get_line_in_buffer(y), x);
}

char current_screen_pos_char() {
    return screen_pos_char(current_buffer->cursor_row, current_buffer->cursor_col);
}

size_t strlen_tab(char* buf) {
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

String* write_buffer = NULL;
void write_respect_tabspace(char* buf, size_t start, size_t count) {
    if (write_buffer == NULL) write_buffer = alloc_String(count);
    else String_clear(write_buffer);
    for (size_t i = 0; i < count; ++i) {
        if (buf[i] == BYTE_TAB) {
            size_t x = Strlen(write_buffer) + start;
            for (int j = x; j < ((x / TAB_WIDTH) + 1) * TAB_WIDTH; ++j) {
                String_push(&write_buffer, ' ');
            }
            continue;
        }
        String_push(&write_buffer, buf[i]);
    }
    write(STDOUT_FILENO, write_buffer->data, Strlen(write_buffer));
}

void editor_make_buffer(char* filename) {
    Buffer* buffer = make_Buffer(filename);
    Vector_push(&buffers, buffer);
}
    
void editor_init(char* filename) {
    current_mode = EM_NORMAL;
    command_buffer = alloc_String(10);
    inplace_make_Vector(&buffers, 10);
    inplace_make_Vector(&active_copy.data, 10);
    current_buffer = make_Buffer(filename);
    Vector_push(&buffers, current_buffer);
    current_buffer_idx = 0;
    editor_top = 1;
    editor_window_size_change();
}

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
    active_insert = make_Edit(current_buffer->undo_index, 
                        Buffer_get_line_index(current_buffer, current_buffer->cursor_row),
                        0, line);
}

void end_insert() {
    char** line_p = get_line_in_buffer(current_buffer->cursor_row);
    free(*line_p);
    *line_p = strdup(active_insert->new_content->data);
    Buffer_push_undo(current_buffer, active_insert);
    active_insert = NULL;
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
                        Buffer_get_line_index(current_buffer, y_pos), 0, line);
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
            char* prev_line = current_buffer->lines.elements[line_num-1];
            Edit* new_action = make_Edit(current_buffer->undo_index, 
                        line_num-1, 0, prev_line);
            String_pop(new_action->new_content); // Remove trailing character, it got de-yeeted
            current_buffer->cursor_col = strlen_tab(new_action->new_content->data);
            current_buffer->natural_col = current_buffer->cursor_col;
            --current_buffer->cursor_row;
            move_to_current();
            write_respect_tabspace(active_insert->new_content->data, current_buffer->cursor_col,
                                    active_insert->new_content->length);
            Strcat(&new_action->new_content, active_insert->new_content);

            // Convert to a delete action
            free(active_insert->new_content);
            active_insert->new_content = NULL;
            Buffer_push_undo(current_buffer, active_insert);
            Vector_delete(&current_buffer->lines, line_num);

            active_insert = new_action;
            display_buffer_rows(y_pos+1, editor_bottom);
            return 1;
        }
        return 0;
    }
    char* current_ptr = line_pos(active_insert->new_content->data, current_buffer->cursor_col-1);
    size_t rest = Strlen(active_insert->new_content)
                    - (current_ptr - active_insert->new_content->data);
    String_delete(active_insert->new_content, current_ptr - active_insert->new_content->data);
    current_buffer->cursor_col = line_pos_ptr(active_insert->new_content->data, current_ptr);
    current_buffer->natural_col = current_buffer->cursor_col;
    move_to_current();
    clear_line();
    write_respect_tabspace(current_ptr, current_buffer->cursor_col, rest+1);
    return 1;
}

void add_chr(char c) {
    if (c == BYTE_ENTER) {
        char** line_p = get_line_in_buffer(current_buffer->cursor_row);
        char* rest = strdup(line_pos(active_insert->new_content->data, current_buffer->cursor_col));
        Edit* save_insert = active_insert;
        char* current_ptr = line_pos(save_insert->new_content->data, current_buffer->cursor_col);
        size_t save_length = current_ptr - save_insert->new_content->data;
        size_t save_col = current_buffer->cursor_col;

        free(*line_p);
        *line_p = strdup(save_insert->new_content->data);
        editor_newline(1, rest);
        free(rest);
        write_respect_tabspace(active_insert->new_content->data, 0, 
                                active_insert->new_content->length);
        save_insert->new_content->length = save_length;
        String_push(&save_insert->new_content, '\n');
        //save_insert->new_content->data[save_col+1] = 0;
        Buffer_push_undo(current_buffer, save_insert);
        move_cursor(current_buffer->cursor_row - 1, save_col);
        clear_line();
        move_to_current();
        return;
    }
    String_insert(&active_insert->new_content,
                    line_pos(active_insert->new_content->data, current_buffer->cursor_col)
                    - active_insert->new_content->data, c);
    clear_line();
    // Refresh, insert might realloc
    char* current_ptr = line_pos(active_insert->new_content->data, current_buffer->cursor_col);
    size_t rest = Strlen(active_insert->new_content)
                    - (current_ptr - active_insert->new_content->data);
    write_respect_tabspace(current_ptr, current_buffer->cursor_col, rest+1);
    current_buffer->cursor_col = line_pos_ptr(active_insert->new_content->data, current_ptr+1);
    current_buffer->natural_col = current_buffer->cursor_col;
    move_to_current();
}

/**
 * "o" command in vim.
 * TODO: make "O".
 * Initial string doesn't need to be malloc'd or anything.
 */
void editor_newline(int side, char* initial) {
    if (side > 0) {
        int y_pos = current_buffer->cursor_row + 1;
        size_t line_num = Buffer_get_line_index(current_buffer, y_pos);
        String* add_space;
        size_t start_len;
        if (PRESERVE_INDENT) {
            char* line = *get_line_in_buffer(current_buffer->cursor_row);
            add_space = alloc_String(10);
            for (char* ptr = line; is_whitespace(*ptr); ++ptr) {
                String_push(&add_space, *ptr);
            }
            start_len = strlen_tab(add_space->data);
            String* rest = make_String(initial);
            Strcat(&add_space, rest);
            free(rest);
            initial = add_space->data;
        }
        Vector_insert(&current_buffer->lines, line_num, strdup(initial));
        active_insert = make_Insert(current_buffer->undo_index, line_num, 0, initial);
        current_buffer->cursor_row += 1;
        current_buffer->cursor_col = 0;
        if (PRESERVE_INDENT) {
            free(add_space);
            current_buffer->cursor_col = start_len;
        }
        current_buffer->natural_col = 0;
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

/**
 * Display rows [start, end] inclusive, in screen coords (1-indexed).
 */
void display_buffer_rows(size_t start, size_t end) {
    //TODO handle line overruns
    //TODO only redraw changes
    for (size_t i = start; i <= end; ++i) {
        move_cursor(i-1, 0);
        clear_line();
        if (Buffer_get_line_index(current_buffer, i-1) < current_buffer->lines.size) {
            char* str;
            if (active_insert != NULL && current_buffer->cursor_row == i-1) {
                str = active_insert->new_content->data;
            }
            else {
                str = *get_line_in_buffer(i-1);
            }
            if (strlen(str) != 0) {
                write_respect_tabspace(str, 0, strlen(str));
            }

        }
    }
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
    if (active_insert != NULL) {
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
    if (active_insert != NULL) {
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
        if (active_insert != NULL) {
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
    if (active_insert != NULL) {
        line = active_insert->new_content->data;
    }
    else {
        line = *get_line_in_buffer(current_buffer->cursor_row);
    }
    if (line_pos(line, current_buffer->cursor_col) != line) {
        if (active_insert == NULL) {
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
    current_buffer->natural_col = current_buffer->cursor_col;
}
void editor_move_right() {
    int max_char = -1;
    char* line;
    if (active_insert != NULL) {
        line = active_insert->new_content->data;
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
        if (active_insert != NULL) {
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
    if (display) {
        display_current_buffer();
    }

    char* line;
    line = *get_line_in_buffer(current_buffer->cursor_row);
    size_t max_col = strlen_tab(line);
    if (current_buffer->cursor_col >= max_col) {
        current_buffer->cursor_col = max_col - 1;
    }
}

void editor_align_tab() {
    if (active_insert != NULL) {
        // Left align in insert mode.
        left_align_tab(active_insert->new_content->data);
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
    if (repaint == RP_ALL) {
        display_current_buffer();
    }
    else if (repaint == RP_LINES) {
        if (ctx->start_row <= ctx->jump_row) {
            display_buffer_rows(1 + ctx->start_row - buf->top_row, 1 + ctx->jump_row - buf->top_row);
        }
        else {
            display_buffer_rows(1 + ctx->jump_row - buf->top_row, 1 + ctx->start_row - buf->top_row);
        }
    }
    else if (repaint == RP_LOWER) {
        display_buffer_rows(1 + ctx->start_row - buf->top_row, window_size.ws_row);
    }
    else if (repaint == RP_UPPER) {
        display_buffer_rows(1, 1 + ctx->start_row - buf->top_row);
    }
    editor_fix_view();
    move_to_current();
}
