#pragma once

#include <stddef.h>

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
 * Vector delete element at index. Shifts everything past it left.
 */
void Vector_delete(Vector* v, size_t idx);

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

