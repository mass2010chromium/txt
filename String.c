#include "String.h"

#include <stdlib.h>
#include <string.h>

String* make_String(const char* data) {
    String* ret = alloc_String(strlen(data));
    ret->length = strlen(data);
    memcpy(ret->data, data, strlen(data)+1);
    return ret;
}

/**
 * Take a malloc'd string and realloc it into a String.
 */
String* convert_String(char* data) {
    size_t maxlen = strlen(data);
    String* ret = realloc(data, sizeof(String) + maxlen + 1);
    memmove(ret + 1, ret, maxlen);
    ret->length = maxlen;
    ret->max_length = maxlen;
    return ret;
}

String* alloc_String(size_t maxlen) {
    String* ret = malloc(sizeof(String) + maxlen + 1);
    ret->data[0] = 0;
    ret->length = 0;
    ret->max_length = maxlen;
    return ret;
}

String* realloc_String(String* s, size_t maxlen) {
    String* ret = realloc(s, sizeof(String) + maxlen + 1);
    ret->max_length = maxlen;
    if (ret->length > maxlen) {
        ret->length = maxlen;
        ret->data[maxlen] = 0;
    }
    return ret;
}

size_t Strlen(String* s) {
    return s->length;
}

/*
 * Might modify the first pointer (length extend).
 * Append string b to string a.
 * Also returns the new pointer if u want to use that instead.
 */
String* Strcat(String** _a, String* b) {
    String* a = *_a;
    String* ret = realloc_String(a, Strlen(a) + Strlen(b));
    memcpy(ret->data + Strlen(ret), b->data, Strlen(b)+1);
    ret->length = ret->max_length;
    *_a = ret;
    return ret;
}

void String_push(String** _s, char c) {
    String* s = *_s;
    if (s->length < s->max_length) {
        s->data[s->length] = c;
        s->data[s->length+1] = 0;
        ++s->length;
    }
    else {
        String* new = realloc_String(s, s->max_length * 2 + 1);
        String_push(&new, c);
        *_s = new;
    }
}

char String_pop(String* s) {
    s->length -= 1;
    char ret = s->data[s->length];
    s->data[s->length] = 0;
    return ret;
}

void String_clear(String* s) {
    s->length = 0;
    s->data[0] = 0;
}

/**
 * Delete character at index given (0-indexed) and returns it.
 * Does not resize (maxlen) string.
 */
char String_delete(String* s, size_t index) {
    size_t line_len = s->length;
    size_t rest = line_len - (index + 1);
    char ret = s->data[index];
    s->length -= 1;
    memmove(s->data + index, s->data + index+1, rest+1);
    return ret;
}

/**
 * Delete characters [a, b).
 * Does not resize (maxlen) string.
 */
void String_delete_range(String* s, size_t a, size_t b) {
    size_t line_len = s->length;
    size_t rest = line_len - b;
    s->length -= (b-a);
    memmove(s->data + a, s->data + b, rest+1);
}

/**
 * Postcondition: (*s)->data[index] == c
 */
void String_insert(String** _s, size_t index, char c) {
    String* s = *_s;
    if (s->length == s->max_length) {
        s = realloc_String(s, s->max_length + 1);
        *_s = s;
    }
    size_t tail = s->length - index;
    s->length += 1;
    memmove(s->data+index+1, s->data+index, tail+1);
    s->data[index] = c;
}
