#pragma once
#include <stddef.h>

struct String {
    size_t length;
    size_t max_length;
    char data[0];
};

typedef struct String String;

String* make_String(const char* data);
String* alloc_String(size_t);
String* realloc_String(String*, size_t);

size_t Strlen(String* s);

String* Strcat(String**, String*);
void String_push(String**, char);
char String_pop(String*);
void String_clear(String*);
char String_delete(String* s, size_t index);

/**
 * Postcondition: (*s)->data[index] == c
 */
void String_insert(String** s, size_t index, char c);
