#pragma once

#include "../Vector.h"

#define ASSERT_VS_EQ(a, b) \
do { \
    ASSERT_EQ((a)->size, (b)->size); \
    for (size_t i = 0; i < (a)->size; ++i) { \
        ASSERT_STREQ((char*) (a)->elements[i], (char*) (b)->elements[i]); \
    } \
} while (0);

void inplace_make_VS(Vector* ret, char** data) {
    inplace_make_Vector(ret, 10);
    while (*data) {
        Vector_push(ret, *data);
        ++data;
    }
}
