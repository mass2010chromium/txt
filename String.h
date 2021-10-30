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
/**
 * Take a malloc'd String and "realloc" it into a string.
 */
char* String_to_cstr(String* data);
String* alloc_String(size_t);
String* realloc_String(String*, size_t);

size_t Strlen(const String* s);

/*
 * Might modify the first pointer (length extend).
 * Append string b to string a.
 * Also returns the new pointer if u want to use that instead.
 */
String* Strcat(String**, const String*);

/**
 * @see Strcat, except second argument is a C string
 */
String* Strcats(String**, const char*);

/**
 * @see Strcats, except only n bytes are copied.
 */
String* Strncats(String**, const char*, size_t);

/**
 * Push a char onto the end of this string. Increases length.
 * Might cause a reallocation.
 */
void String_push(String**, char);

/**
 * Removes a char from the end of this string and returns it.
 * Decreases length. Will not realloc.
 */
char String_pop(String*);

/**
 * Removes all chars from this string.
 * Decreases length. Will not realloc.
 * To free memory you have to free() the String.
 */
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

/**
 * Postcondition: (*s)->data[index:index+strlen(insert)] == insert
 */
void String_inserts(String** s, size_t index, const char* insert);

/**
 * Postcondition: (*s)->data[index:index+n] == insert[:n]
 */
void String_ninserts(String** s, size_t index, const char* insert, size_t n);

/**
 * "Fit" the string (realloc to fit length).
 * Use:
 *      String* s = ...;
 *      s = String_fit(s);
 */
String* String_fit(String* s);
