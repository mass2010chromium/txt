#include <termios.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>

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

struct winsize window_size;

Buffer* current_buffer;

Vector/*Buffer* */ buffers;
Vector/*char*/ command_buffer;
EditorAction* active_insert = NULL;
size_t insert_y = (size_t) -1;
size_t insert_x = (size_t) -1;

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
        if ( !ret ) {
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
   
    return 0;
}

void move_cursor(size_t y, size_t x) {
    printf("\033[%d;%dH", y, x);
}
    
void editor_init(char* filename) {
    inplace_make_Vector(&command_buffer, 10);
    inplace_make_Vector(&buffers, 10);
    current_buffer = make_Buffer(filename);
    Vector_push(&buffers, current_buffer);
}

size_t screen_pos_to_file_pos(size_t y, size_t x, size_t* line_start) {
    size_t start = current_buffer->top_left_file_pos;
    for (size_t r = 0; r < y-1; ++r) {
        start += strlen(current_buffer->lines.elements[r]);
    }
    *line_start = start;
    start += (x - 1);
    return start;
}

void begin_insert() {
    size_t x, y;
    get_cursor_pos(&y, &x);
    size_t line_start;
    size_t cursor_pos = screen_pos_to_file_pos(y, x, &line_start);
    char* line = current_buffer->lines.elements[y-1];
    size_t line_len = strlen(line);
    EditorContext ctx;
    resolve_index(&current_buffer->root, line_start, &ctx);
    EditorAction* remove_line = make_EditorAction(ctx.index, line_len, ED_DELETE);
    EditorAction* insert_line = make_EditorAction(ctx.index, line_len, ED_INSERT);
    EditorAction_add_child(ctx.action, remove_line);
    EditorAction_add_child(ctx.action, insert_line);
    remove_line->buf = strdup(line);
    insert_line->buf = line;
    active_insert = insert_line;
    insert_y = y-1;
    insert_x = x-1;
}

int del_chr() {
    if (insert_x == 0) return 0;
    char* line = current_buffer->lines.elements[insert_y];
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

void add_chr() {

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
    for (size_t i = 1; i < window_size.ws_row && i <= current_buffer->lines.size; ++i) {
        move_cursor(i, 1);
        clear_line();
        write(STDOUT_FILENO, current_buffer->lines.elements[i-1],
                        strlen(current_buffer->lines.elements[i-1]) - 1);
    }
    move_cursor(current_buffer->cursor_row, current_buffer->cursor_col);
}
