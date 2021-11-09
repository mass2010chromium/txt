#pragma once

void __debug_init();
void print(const char* format, ...);

#ifndef DEBUG
#define fprintf(...) (void)(__VA_ARGS__)
#endif
