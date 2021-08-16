#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "utils.h"

/**
 * Make an empty deque with a specified max size.
 */
Deque* make_deque(size_t size) {
    //TODO error checking
    Deque* ret = malloc(sizeof(Deque));
    inplace_make_Deque(ret, size);
    return ret;
}

void inplace_make_Deque(Deque* deque, size_t size) {
    deque->head = 0;
    deque->tail = 0;
    deque->size = 0;
    deque->max_size = size;
    deque->elements = malloc(size * sizeof(void*));
}

size_t Deque_size(Deque* this) {
    return this->size;
}

bool Deque_full(Deque* this) {
    return this->size == this->max_size;
}

bool Deque_empty(Deque* this) {
    return this->size == 0;
}

/**
 * Push onto the end of the deque.
 */
int Deque_push(Deque* this, void* element) {
    if (Deque_full(this)) {
        return -1;
    }
    this->elements[this->tail] = element;
    this->tail = (this->tail + 1) % this->max_size;
    ++this->size;
    return 0;
}

/**
 * Push onto the front
 */
int Deque_push_l(Deque* this, void* element) {
    if (Deque_full(this)) {
        return -1;
    }
    --this->head;
    if (this->head < 0) { this->head = this->max_size - 1; }
    this->elements[this->head] = element;
    ++this->size;
    return 0;
}

/**
 * Pop from the front of the deque.
 */
void* Deque_pop(Deque* this) {
    if (Deque_empty(this)) {
        //TODO error handling?
        return NULL;
    }
    void* elt = this->elements[this->head];
    this->head = (this->head + 1) % this->max_size;
    --this->size;
    return elt;
}

/**
 * Pop from the end of the deque.
 */
void* Deque_pop_r(Deque* this) {
    if (Deque_empty(this)) {
        //TODO error handling?
        return NULL;
    }
    --this->tail;
    if (this->tail < 0) { this->tail = this->max_size - 1; }
    void* elt = this->elements[this->tail];
    --this->size;
    return elt;
}

void Deque_destroy(Deque* this) {
    free(this->elements);
    this->elements = NULL;
}

Vector* make_Vector(size_t init_size) {
    //TODO error checking
    Vector* ret = malloc(sizeof(Vector));
    inplace_make_Vector(ret, init_size);
    return ret;
}

void inplace_make_Vector(Vector* vector, size_t init_size) {
    vector->size = 0;
    vector->max_size = init_size;
    vector->elements = malloc(init_size * sizeof(void*));
}

/**
 * Vector shallow copy.
 */
Vector* Vector_copy(Vector* v) {
    // TODO error handling
    Vector* ret = malloc(sizeof(Vector));
    ret->size = v->size;
    ret->max_size = v->max_size;
    ret->elements = malloc(v->max_size * sizeof(void*));
    memcpy(ret->elements, v->elements, v->size * sizeof(void*));
    return ret;
}

/**
 * Vector push onto end.
 */
void Vector_push(Vector* v, void* element) {
    if (v->size == v->max_size) {
        // TODO error handling
        v->elements = realloc(v->elements, v->max_size*2 * sizeof(void*));
        v->max_size *= 2;
    }
    v->elements[v->size] = element;
    v->size += 1;
}

/**
 * Vector insert into middle.
 * Postcondition: v[idx] = element
 */
void Vector_insert(Vector* v, size_t idx, void* element) {
    if (v->size == v->max_size) {
        // TODO error handling
        v->elements = realloc(v->elements, v->max_size*2 * sizeof(void*));
        v->max_size *= 2;
    }
    memmove(v->elements + idx + 1, v->elements + idx, (v->size - idx) * sizeof(void*));
    v->elements[idx] = element;
    v->size += 1;
}

void _Vector_quicksort(Vector* v, size_t lower, size_t upper, int(*cmp)(void*, void*)) {
    if (upper - lower <= 1) return;
    void** arr = v->elements;
    // Improve performance on sorted lists
    size_t pivot_idx = (upper + lower) / 2;
    void* pivot = arr[pivot_idx];
    arr[pivot_idx] = arr[lower];
    size_t start = lower + 1;
    size_t end = upper;
    while (start != end) {
        if (cmp(arr[start], pivot) > 0) {
            --end;
            void* end_elt = arr[end];
            arr[end] = arr[start];
            arr[start] = end_elt;
        }
        else {
            arr[start-1] = arr[start];
            ++start;
        }
    }
    arr[start-1] = pivot;
    _Vector_quicksort(v, lower, start-1, cmp);
    _Vector_quicksort(v, start, upper, cmp);
}
void Vector_sort(Vector* v, int(*cmp)(void*, void*)) {
    _Vector_quicksort(v, 0, v->size, cmp);
}

int signed_compare(void* a, void* b) {
    return ((ssize_t) a) > ((ssize_t) b);
}
int unsigned_compare(void* a, void* b) {
    return ((size_t) a) > ((size_t) b);
}

void Vector_clear(Vector* v, size_t init_size) {
    v->size = 0;
    v->max_size = init_size;
    v->elements = realloc(v->elements, init_size * sizeof(void*));
}

void Vector_destroy(Vector* this) {
    free(this->elements);
    this->elements = NULL;
}

size_t fwriteall(void* buf, size_t size, size_t n_element, FILE* f) {
    // TODO technically dangerous but idrc
    size_t total = size * n_element;
    size_t total_written = 0;
    while (total_written != total) {
        size_t n_written = fwrite(buf, 1, total, f);
        if (n_written == 0) {
            return total_written;
        }
        total_written += n_written;
        buf += n_written;
        total -= n_written;
    }
    return total_written;
}

size_t fcopy(FILE* dest, FILE* src, size_t bytes) {
    size_t total_copied = 0;
    const size_t BLOCKSIZE = 4096;
    char buf[BLOCKSIZE];
    while (total_copied != bytes) {
        size_t copy_block = bytes;
        if (copy_block > BLOCKSIZE) copy_block = BLOCKSIZE;
        size_t num_read = fread(buf, sizeof(char), copy_block, src);
        if (num_read == 0) {
            return total_copied;
        }
        fwriteall(buf, sizeof(char), num_read, dest);
        total_copied += num_read;
    }
    return total_copied;
}
