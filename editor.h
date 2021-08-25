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

Buffer* current_buffer;

Vector/*Buffer* */ buffers;
Vector/*char*/ command_buffer;
EditorAction* active_insert;
size_t insert_y;
size_t insert_x;

void clear_line();
void clear_screen();

int get_cursor_pos(size_t *y, size_t *x);

void move_cursor(size_t y, size_t x);

void editor_init(char* filename);

char** get_line_in_buffer(size_t y);

size_t screen_pos_to_file_pos(size_t y, size_t x, size_t* line_start);

void begin_insert();
void end_insert();

int del_chr();

void add_chr(char c);

void display_bottom_bar(char* left, char* right);

void display_current_buffer();
