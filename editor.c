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
const char BYTE_ENTER       = '\012';     // Line Feed
const char BYTE_BACKSPACE   = '\177';
const char BYTE_UPARROW     = '\110';
const char BYTE_LEFTARROW   = '\113';
const char BYTE_RIGHTARROW  = '\115';
const char BYTE_DOWNARROW   = '\120';
const char BYTE_DELETE      = '\123';

typedef int EditorMode;
const EditorMode EM_QUIT    = -1;
const EditorMode EM_NORMAL  = 0;
const EditorMode EM_INSERT  = 1;
const EditorMode EM_COMMAND = 2;

struct winsize window_size = {0};

Buffer* current_buffer = NULL;

Vector/*Buffer* */ buffers = {0};
Vector/*char*/ command_buffer = {0};
EditorAction* active_insert = NULL;
size_t insert_y = (size_t) -1;
size_t insert_x = (size_t) -1;
size_t undo_index = 0;
size_t editor_top;
size_t editor_bottom;

void editor_window_size_change() {
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &window_size);
    editor_bottom = window_size.ws_row - 1;
}

void new_action() {
    undo_index += 1;
}

void clear_line() {
    write(STDOUT_FILENO, "\033[0K", 4);
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
    if (i > 29) {
        int* x = 0;
        *x = 1;
    }
    if (i < 2) {
        return 1;
    }
    for ( i -= 2, pow = 1; buf[i] != ';'; i--, pow *= 10)
        *x = *x + ( buf[i] - '0' ) * pow;
   
    for ( i-- , pow = 1; buf[i] != '['; i--, pow *= 10)
        *y = *y + ( buf[i] - '0' ) * pow;
   
    return 0;
}

void move_cursor(size_t y, size_t x) {
    char buf[60] = {0};
    sprintf(buf, "\033[%lu;%luH", y, x);
    write(STDOUT_FILENO, buf, strlen(buf));
}
    
void editor_init(char* filename) {
    current_mode = EM_NORMAL;
    inplace_make_Vector(&command_buffer, 10);
    inplace_make_Vector(&buffers, 10);
    current_buffer = make_Buffer(filename);
    Vector_push(&buffers, current_buffer);
    editor_top = 1;
    editor_window_size_change();
}

char** get_line_in_buffer(size_t y) {
    return Buffer_get_line(current_buffer, y);
}

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
    size_t line_start;
    size_t cursor_pos = screen_pos_to_file_pos(current_buffer->cursor_row,
                                                current_buffer->cursor_col, &line_start);
    char* line = *get_line_in_buffer(current_buffer->cursor_row - 1);
    size_t line_len = strlen(line);
    active_insert = make_EditorAction(undo_index, line_start, line);
    insert_y = current_buffer->cursor_row-1;
    insert_x = current_buffer->cursor_col-1;
}
void end_insert() {
    char** line_p = get_line_in_buffer(insert_y);
    free(*line_p);
    *line_p = strdup(active_insert->new_content);
    Deque_push(&current_buffer->undo_buffer, active_insert);
    active_insert = NULL;
    insert_y = (size_t) -1;
    insert_x = (size_t) -1;
}

int del_chr() {
    if (insert_x == 0) return 0;
    char* line = active_insert->new_content;
    size_t line_len = strlen(line);
    size_t rest = line_len - insert_x;
    memmove(line + insert_x - 1, line + insert_x, rest+1);
    insert_x -= 1;
    current_buffer->cursor_col = insert_x+1;
    current_buffer->natural_col = insert_x+1;
    move_cursor(insert_y+1, insert_x+1);
    clear_line();
    write(STDOUT_FILENO, line+insert_x, rest);
    return 1;
}

void add_chr(char c) {
    char* line = active_insert->new_content;
    size_t line_len = strlen(line);
    size_t rest = line_len - insert_x;
    line = string_insert_c(line, insert_x, c);
    active_insert->new_content = line;
    clear_line();
    write(STDOUT_FILENO, line+insert_x, rest+1);
    insert_x += 1;
    current_buffer->cursor_col = insert_x+1;
    current_buffer->natural_col = insert_x+1;
    move_cursor(insert_y+1, insert_x+1);
}

void display_bottom_bar(char* left, char* right) {
    size_t x, y;
    get_cursor_pos(&y, &x);
    move_cursor(window_size.ws_row, 0);
    write(STDOUT_FILENO, left, strlen(left));
    write(STDOUT_FILENO, "\0", 1);
    if (right != NULL) {

    }
    print("Displayed bottom bar [%s]\n", left);
    clear_line();
    move_cursor(y, x);
}

void display_current_buffer() {
    //TODO handle line overruns
    //TODO only redraw changes
    for (size_t i = editor_top; i <= editor_bottom; ++i) {
        move_cursor(i, 1);
        clear_line();
        char* str = *get_line_in_buffer(i-1);
        write(STDOUT_FILENO, str, strlen(str) - 1);
    }
    move_cursor(current_buffer->cursor_row, current_buffer->cursor_col);
}

void editor_move_up() {
    bool insert = false;
    if (active_insert != NULL) {
        insert = true;
        end_insert();
    }
    if (current_buffer->cursor_row > editor_top) { current_buffer->cursor_row -= 1; }
    else {
        Buffer_scroll(current_buffer, editor_bottom+1-editor_top, -1);
        display_current_buffer();
    }
    size_t col = strlen(*get_line_in_buffer(current_buffer->cursor_row-1)) - 1;
    if (col > current_buffer->natural_col) { col = current_buffer->natural_col; }
    current_buffer->cursor_col = col;
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
    if (current_buffer->cursor_row < editor_bottom) { current_buffer->cursor_row += 1; }
    else {
        Buffer_scroll(current_buffer, editor_bottom+1-editor_top, 1);
        display_current_buffer();
        print("buffer top row %lu\n", current_buffer->top_row);
    }
    size_t col = strlen(*get_line_in_buffer(current_buffer->cursor_row-1)) - 1;
    if (col > current_buffer->natural_col) { col = current_buffer->natural_col; }
    current_buffer->cursor_col = col;
    if (insert) {
        begin_insert();
    }
}
void editor_move_left() {
    if (current_buffer->cursor_col > 1) {
        current_buffer->cursor_col -= 1;
        if (active_insert != NULL) insert_x -= 1;
    }
    current_buffer->natural_col = current_buffer->cursor_col;
}
void editor_move_right() {
    int insert_offset = 0;
    if (active_insert != NULL) insert_offset += 1;
    if (current_buffer->cursor_col < window_size.ws_col
            && current_buffer->cursor_col < insert_offset
                + strlen(*get_line_in_buffer(current_buffer->cursor_row-1))-1) {
        current_buffer->cursor_col += 1;
        if (active_insert != NULL) insert_x += 1;
    }
    current_buffer->natural_col = current_buffer->cursor_col;
}
