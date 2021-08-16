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

void clear_line() {
    write(STDOUT_FILENO, "\033[0K", 4);
}

void clear_screen() {
    write(STDOUT_FILENO, "\033[2J", 4);
}
    
void editor_init(char* filename) {
    inplace_make_Vector(&command_buffer, 10);
    inplace_make_Vector(&buffers, 10);
    current_buffer = make_Buffer(filename);
    Vector_push(&buffers, current_buffer);
}

size_t screen_pos_to_file_pos(int y, int x, size_t* line_start) {
    size_t start = current_buffer->top_left_file_pos;
    for (int r = 0; r < y-1; ++r) {
        start += strlen(current_buffer->lines.elements[r]);
    }
    *line_start = start;
    start += (x - 1);
    return start;
}

void begin_insert() {
    int x, y;
    get_cursor_pos(y, x);
    size_t line_start;
    size_t cursor_pos = screen_pos_to_file_pos(y, x, &line_start);
}

/**
 * Adapted from https://stackoverflow.com/questions/50884685/how-to-get-cursor-position-in-c-using-ansi-code
 * See: https://en.wikipedia.org/wiki/ANSI_escape_code (Device Status Report)
 */
int get_cursor_pos(int *y, int *x) {
    char buf[30] = {0};
    int ret, i, pow;
    char ch;
    *y = 0;
    *x = 0;
   
    struct termios term, restore;
   
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

void move_cursor(int y, int x) {
    printf("\033[%d;%dH", y, x);
}

void display_bottom_bar(char* left, char* right) {
    int x, y;
    get_cursor_pos(&y, &x);
    move_cursor(window_size.ws_row, 0);
    write(STDOUT_FILENO, left, strlen(left));
    write(STDOUT_FILENO, "\0", 1);
    if (right != NULL) {

    }
    print("Displayed bottom bar [%s]\n", left);
    move_cursor(y, x);
}

void display_current_buffer() {
    //TODO handle line overruns
    //TODO only redraw changes
    for (int i = 1; i < window_size.ws_row && i <= current_buffer->lines.size; ++i) {
        move_cursor(i, 1);
        clear_line();
        write(STDOUT_FILENO, current_buffer->lines.elements[i-1],
                        strlen(current_buffer->lines.elements[i-1]) - 1);
    }
    move_cursor(current_buffer->cursor_row, current_buffer->cursor_col);
}
