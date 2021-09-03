#pragma once
#include <stddef.h>

struct String {
    size_t length;
    size_t max_length;
    char data[0];
};

typedef struct String String;

String* make_String(const char* data);
/**
 * Take a malloc'd string and realloc it into a String.
 */
String* convert_String(char* data);
String* alloc_String(size_t);
String* realloc_String(String*, size_t);

size_t Strlen(String* s);

/*
 * Might modify the first pointer (length extend).
 * Append string b to string a.
 * Also returns the new pointer if u want to use that instead.
 */
String* Strcat(String**, String*);
void String_push(String**, char);
char String_pop(String*);
void String_clear(String*);

/**
 * Delete character at index given (0-indexed) and returns it.
 * Does not resize (maxlen) string.
 */
char String_delete(String* s, size_t index);

/**
 * Delete characters [a, b).
 * Does not resize (maxlen) string.
 */
void String_delete_range(String* s, size_t a, size_t b);

/**
 * Postcondition: (*s)->data[index] == c
 */
void String_insert(String** s, size_t index, char c);
