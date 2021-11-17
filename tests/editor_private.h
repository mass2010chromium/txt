#pragma once
extern bool SCREEN_WRITE;
extern int TAB_WIDTH;

size_t tab_round_up(size_t start);
size_t line_pos_ptr(const char* buf, const char* ptr);
size_t strlen_tab(const char*);