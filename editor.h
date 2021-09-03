#pragma once
#include <sys/ioctl.h>

#include "buffer.h"
#include "utils.h"

const char BYTE_ESC;
//const char BYTE_ENTER;     // Carriage Return
const char BYTE_ENTER;     // Line Feed
const char BYTE_BACKSPACE;
const char BYTE_UPARROW;
const char BYTE_LEFTARROW;
const char BYTE_RIGHTARROW;
const char BYTE_DOWNARROW;
const char BYTE_DELETE;

typedef int EditorMode;
extern const EditorMode EM_QUIT;
extern const EditorMode EM_NORMAL;
extern const EditorMode EM_INSERT;
extern const EditorMode EM_COMMAND;

EditorMode current_mode;

struct winsize window_size;

Copy active_copy;

Buffer* current_buffer;

Vector/*Buffer* */ buffers;
String* command_buffer;
Edit* active_insert;
size_t editor_top;
size_t editor_bottom;

void editor_window_size_change();

void editor_new_action();

int editor_undo();

void clear_line();
void clear_screen();

int get_cursor_pos(size_t *y, size_t *x);

void move_to_current();
void move_cursor(size_t y, size_t x);

/**
 * Pointer corresponding to the spot in buf at screen pos x. (0 indexed)
 */
char* line_pos(char* buf, ssize_t x);

void editor_init(char* filename);

char** get_line_in_buffer(size_t y);

void begin_insert();
void end_insert();

int editor_backspace();

void add_chr(char c);

void editor_newline(int, char*);

void display_bottom_bar(char* left, char* right);

void display_buffer_rows(size_t start, size_t end);

void display_current_buffer();

void left_align_tab(char* line);
void right_align_tab(char* line);

void editor_move_up();
void editor_move_down();
/**
 * Only works in not insert mode.
 */
void editor_move_EOL();
void editor_move_left();
void editor_move_right();
void editor_fix_view();
void editor_align_tab();

void editor_move_to(ssize_t row, ssize_t col);

void editor_repaint(RepaintType repaint, EditorContext* ctx);
