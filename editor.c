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
String* command_buffer = NULL;
EditorAction* active_insert = NULL;
size_t undo_index = 0;
size_t editor_top;
size_t editor_bottom;

void editor_window_size_change() {
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &window_size);
    editor_bottom = window_size.ws_row - 1;
}

void editor_new_action() {
    undo_index += 1;
}

int editor_undo() {
    int num_undo = Buffer_undo(current_buffer, undo_index);
    if (num_undo > 0) {
        undo_index -= 1;
        display_current_buffer();
    }
    return num_undo;
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

void move_to_current() {
    move_cursor(current_buffer->cursor_row, current_buffer->cursor_col);
}
    
void editor_init(char* filename) {
    current_mode = EM_NORMAL;
    command_buffer = alloc_String(10);
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
    //size_t line_start;
    //size_t cursor_pos = screen_pos_to_file_pos(current_buffer->cursor_row,
    //                                            current_buffer->cursor_col, &line_start);
    char* line = *get_line_in_buffer(current_buffer->cursor_row - 1);
    //size_t line_len = strlen(line);
    active_insert = make_EditorAction(undo_index, 
                        Buffer_get_line_index(current_buffer, current_buffer->cursor_row - 1),
                        0, line);
}

void end_insert() {
    char** line_p = get_line_in_buffer(current_buffer->cursor_row-1);
    free(*line_p);
    *line_p = strdup(active_insert->new_content->data);
    Buffer_push_undo(current_buffer, active_insert);
    active_insert = NULL;
}

/**
 * Delete a character under the cursor (except newlines).
 */
int del_chr() {
    size_t y_pos = current_buffer->cursor_row-1;
    size_t x_pos = current_buffer->cursor_col-1;
    char** lineptr = (char**) get_line_in_buffer(y_pos);
    char* line = *lineptr;
    if (strlen(line) == 0) {
        return 0;
    }
    else if (strlen(line) == 1 && line[0] == '\n') {
        return 0;
    }
    EditorAction* delete_action = make_EditorAction(undo_index, 
                        Buffer_get_line_index(current_buffer, y_pos), 0, line);
    size_t rest = Strlen(delete_action->new_content) - x_pos;
    String_delete(delete_action->new_content, x_pos);
    clear_line();
    write(STDOUT_FILENO, delete_action->new_content->data + x_pos, rest);
    free(line);
    *lineptr = strdup(delete_action->new_content->data);
    Buffer_push_undo(current_buffer, delete_action);
    return 1;
}

int editor_backspace() {
    int y_pos = current_buffer->cursor_row-1;
    size_t line_num = Buffer_get_line_index(current_buffer, y_pos);
    //printf("BBB\n");
    if (current_buffer->cursor_col == 1) {
        //printf("AAAAAAAAAAA\n");
        if (y_pos == 0) {

        }
        if (y_pos > 0) {
            char* prev_line = *get_line_in_buffer(y_pos-1);
            EditorAction* new_action = make_EditorAction(undo_index, 
                        line_num-1, 0, prev_line);
            new_action->new_content->length -= 1; // Remove trailing character, it got de-yeeted
            current_buffer->cursor_col = 1 + new_action->new_content->length;
            current_buffer->natural_col = current_buffer->cursor_col;
            --current_buffer->cursor_row;
            move_to_current();
            //clear_line();
            write(STDOUT_FILENO, active_insert->new_content->data, 
                                    active_insert->new_content->length);
            //printf("%d\n", new_action->new_content->length);
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
    size_t rest = Strlen(active_insert->new_content) - (current_buffer->cursor_col - 1);
    String_delete(active_insert->new_content, current_buffer->cursor_col - 2);
    --current_buffer->cursor_col;
    current_buffer->natural_col = current_buffer->cursor_col;
    move_to_current();
    clear_line();
    write(STDOUT_FILENO, active_insert->new_content->data+(current_buffer->cursor_col - 1), rest);
    return 1;
}

void add_chr(char c) {
    if (c == BYTE_ENTER) {
        char** line_p = get_line_in_buffer(current_buffer->cursor_row - 1);
        char* rest = active_insert->new_content->data + current_buffer->cursor_col - 1;
        EditorAction* save_insert = active_insert;
        size_t save_col = current_buffer->cursor_col - 1;
        editor_newline(1, rest);
        write(STDOUT_FILENO, active_insert->new_content->data, active_insert->new_content->length);
        save_insert->new_content->length = save_col;
        String_push(&save_insert->new_content, '\n');
        //save_insert->new_content->data[save_col+1] = 0;
        free(*line_p);
        *line_p = strdup(save_insert->new_content->data);
        Buffer_push_undo(current_buffer, save_insert);
        move_cursor(current_buffer->cursor_row - 1, save_col+1);
        clear_line();
        move_to_current();
        return;
    }
    size_t rest = Strlen(active_insert->new_content) - (current_buffer->cursor_col - 1);
    String_insert(&active_insert->new_content, current_buffer->cursor_col - 1, c);
    clear_line();
    write(STDOUT_FILENO, active_insert->new_content->data+(current_buffer->cursor_col - 1), rest+1);
    ++current_buffer->cursor_col;
    current_buffer->natural_col = current_buffer->cursor_col;
    move_to_current();
}

/**
 * "o" command in vim.
 * TODO: make "O".
 * Initial string doesn't need to be malloc'd or anything.
 */
void editor_newline(int side, char* initial) {
    int y_pos = current_buffer->cursor_row;
    size_t line_num = Buffer_get_line_index(current_buffer, y_pos);
    current_buffer->cursor_row += 1;
    Vector_insert(&current_buffer->lines, line_num, strdup("\n"));
    //size_t line_len = strlen(line);
    active_insert = make_InsertAction(undo_index, line_num, 0, initial);
    current_buffer->cursor_col = 1;
    current_buffer->natural_col = 1;
    display_buffer_rows(y_pos+1, editor_bottom);
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

void display_buffer_rows(size_t start, size_t end) {
    //TODO handle line overruns
    //TODO only redraw changes
    for (size_t i = start; i <= end; ++i) {
        move_cursor(i, 1);
        clear_line();
        if (Buffer_get_line_index(current_buffer, i-1) < current_buffer->lines.size) {
            char* str = *get_line_in_buffer(i-1);
            if (strlen(str) != 0) {
                write(STDOUT_FILENO, str, strlen(str));
            }

        }
    }
    move_cursor(current_buffer->cursor_row, current_buffer->cursor_col);
}

void display_current_buffer() {
    display_buffer_rows(editor_top, editor_bottom);
}

void editor_move_up() {
    bool insert = false;
    if (active_insert != NULL) {
        insert = true;
        end_insert();
    }
    current_buffer->cursor_row -= 1;
    editor_fix_view();
    size_t col = strlen(*get_line_in_buffer(current_buffer->cursor_row-1)) - 1;
    if (col > current_buffer->natural_col) { col = current_buffer->natural_col; }
    if (col == 0) { col = 1; }
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
    current_buffer->cursor_row += 1;
    editor_fix_view();
    //printf("%d %d %d\n", current_buffer->lines.size, current_buffer->cursor_row, current_buffer->top_row);
    size_t col = strlen(*get_line_in_buffer(current_buffer->cursor_row-1)) - 1;
    if (col > current_buffer->natural_col) { col = current_buffer->natural_col; }
    if (col == 0) { col = 1; }
    current_buffer->cursor_col = col;
    if (insert) {
        begin_insert();
    }
}
void editor_move_left() {
    if (current_buffer->cursor_col > 1) {
        current_buffer->cursor_col -= 1;
    }
    current_buffer->natural_col = current_buffer->cursor_col;
}
void editor_move_right() {
    int max_x;
    char* line;
    if (active_insert != NULL) {
        max_x = Strlen(active_insert->new_content) + 1;
        line = active_insert->new_content->data;
    }
    else {
        max_x = strlen(*get_line_in_buffer(current_buffer->cursor_row-1));
        line = *get_line_in_buffer(current_buffer->cursor_row - 1);
    }
    // Save newlines, but don't count them towards line length for cursor purposes.
    if (strlen(line) != 0 && line[strlen(line) - 1] == '\n') {
        max_x -= 1;
    }
    if (current_buffer->cursor_col < window_size.ws_col
            && current_buffer->cursor_col < max_x) {
        current_buffer->cursor_col += 1;
    }
    current_buffer->natural_col = current_buffer->cursor_col;
}

void editor_fix_view() {
    if (current_buffer->cursor_row > editor_bottom
            || current_buffer->cursor_row > current_buffer->lines.size) {
        current_buffer->cursor_row -= 1;
        Buffer_scroll(current_buffer, editor_bottom+1-editor_top, 1);
        display_current_buffer();
    }
    if (current_buffer->cursor_row < editor_top) {
        current_buffer->cursor_row += 1;
        Buffer_scroll(current_buffer, editor_bottom+1-editor_top, -1);
        display_current_buffer();
    }
}
