#pragma once

#include "../structures/Vector.h"

static char* infile_dat[8] = {
"aaaaaaaaaaaaaaaaaaaaaaaaaaaaaa\n",
"bbbbbbbbbbbbbbbbbbbbbbbbbbbbbb\n",
"cccccccccccccccccccccccccccccc\n",
"dddddddddddddddddddddddddddddd\n",
"eeeeeeeeeeeeeeeeeeeeeeeeeeeeee\n",
"ffffffffffffffffffffffffffffff\n",
"",
NULL
};

#define TESTFILE_LEN 7

#define ASSERT_VS_EQ(a, b) \
do { \
    ASSERT_EQ((a)->size, (b)->size); \
    for (size_t i = 0; i < (a)->size; ++i) { \
        ASSERT_STREQ(((String*) (a)->elements[i])->data, ((String*) (b)->elements[i])->data); \
    } \
} while (0);

void inplace_make_VS(Vector* ret, char** data) {
    inplace_make_Vector(ret, 10);
    while (*data) {
        Vector_push(ret, make_String(*data));
        ++data;
    }
}
