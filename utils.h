#pragma once
#include <stdbool.h>
#include <stdio.h>

struct Deque {
    size_t head;
    size_t tail;   // Past-end: insert at tail index
    size_t size;
    size_t max_size;
    void** elements;
};

typedef struct Deque Deque;

/**
 * Make an empty deque with a specified max size.
 */
Deque* make_Deque(size_t size);

void inplace_make_Deque(Deque* deque, size_t size);

size_t Deque_size(Deque* this);

bool Deque_full(Deque* this);

bool Deque_empty(Deque* this);

/**
 * Push onto the end of the deque.
 */
int Deque_push(Deque* this, void* element);

/**
 * Push onto the front
 */
int Deque_push_l(Deque* this, void* element);

/**
 * Pop from the front of the deque.
 */
void* Deque_pop(Deque* this);

/**
 * Pop from the end of the deque.
 */
void* Deque_pop_r(Deque* this);
void Deque_destroy(Deque* v);

struct Vector {
    size_t size;
    size_t max_size;
    void** elements;
};

typedef struct Vector Vector;

Vector* make_Vector(size_t init_size);

void inplace_make_Vector(Vector* vector, size_t init_size);

/**
 * Vector shallow copy.
 */
Vector* Vector_copy(Vector* v);

/**
 * Vector push onto end.
 */
void Vector_push(Vector* v, void* element);

/**
 * Vector insert into middle.
 * Postcondition: v[idx] = element
 */
void Vector_insert(Vector* v, size_t idx, void* element);

/**
 * Quicksort. Works (probably).
 * Not wrapping qsort for less pointery-ness.
 * Compare function: cmp(a, b) returns "a - b".
 */
void Vector_sort(Vector* v, int(*cmp)(void*, void*));

void Vector_clear(Vector* v, size_t init_size);
void Vector_destroy(Vector* v);

int signed_compare(void*, void*);
int unsigned_compare(void*, void*);

/**
 * MISC
 */
size_t fwriteall(void* buf, size_t size, size_t n_element, FILE* f);
size_t fcopy(FILE* dest, FILE* src, size_t bytes);

char* string_insert_c(char* head, size_t index, char c);
