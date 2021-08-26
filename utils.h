#pragma once
#include <stdbool.h>
#include <stdio.h>

#include "Deque.h"
#include "Vector.h"
#include "String.h"

size_t fwriteall(void* buf, size_t size, size_t n_element, FILE* f);
size_t fcopy(FILE* dest, FILE* src, size_t bytes);

char* string_insert_c(char* head, size_t index, char c);
