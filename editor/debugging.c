#include "debugging.h"

//#define DEBUG 1

#ifdef DEBUG
#include <stdio.h>
#include <stdarg.h>
FILE* __of;
void __debug_init() {
    __of = fopen("log.txt", "w");
}
void print(const char* format, ...) {
    va_list ptr;
    va_start(ptr, format);
    vfprintf(__of, format, ptr);
    fflush(__of);
}
#else
void __debug_init() {}
void print(const char* format, ...) {}
#endif
