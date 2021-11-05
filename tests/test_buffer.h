#pragma once

#include "../common.h"
#include "../buffer.h"
#include "test_utils.h"

UTEST(Buffer, read_file_break_lines) {
    Vector ret;
    inplace_make_Vector(&ret, 10);
    FILE* infile = fopen("./tests/testfile", "r");
    ASSERT_NE(NULL, infile);
    read_file_break_lines(&ret, infile);
    fclose(infile);
    Vector_clear_free(&ret, 10);
    Vector_destroy(&ret);
}
