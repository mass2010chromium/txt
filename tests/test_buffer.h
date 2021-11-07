#pragma once

#include "../common.h"
#include "../buffer.h"
#include "test_utils.h"

char* infile_dat[8] = {
"aaaaaaaaaaaaaaaaaaaaaaaaaaaaaa\n",
"bbbbbbbbbbbbbbbbbbbbbbbbbbbbbb\n",
"cccccccccccccccccccccccccccccc\n",
"dddddddddddddddddddddddddddddd\n",
"eeeeeeeeeeeeeeeeeeeeeeeeeeeeee\n",
"ffffffffffffffffffffffffffffff\n",
"",
NULL
};

UTEST(Buffer_util, read_file_break_lines) {
    Vector ret;
    Vector expected;
    inplace_make_VS(&expected, infile_dat);

    inplace_make_Vector(&ret, 10);
    FILE* infile = fopen("./tests/testfile", "r");
    ASSERT_NE(NULL, infile);
    read_file_break_lines(&ret, infile);
    fclose(infile);

    ASSERT_VS_EQ(&expected, &ret);

    Vector_clear_free(&ret, 10);
    Vector_destroy(&ret);
    Vector_destroy(&expected);
}

UTEST(Buffer, create_destroy) {
    Buffer buf;
    Vector expected;

    inplace_make_Buffer(&buf, "./tests/testfile");
    inplace_make_VS(&expected, infile_dat);

    ASSERT_STREQ("./tests/testfile", buf.name);
    ASSERT_EQ(0, buf.top_row);
    ASSERT_EQ(0, buf.cursor_row);
    ASSERT_EQ(0, buf.cursor_col);
    ASSERT_VS_EQ(&expected, &buf.lines);
    
    Vector_destroy(&expected);
    Buffer_destroy(&buf);
}

UTEST(Buffer, get_num_lines) {
    Buffer buf;
    inplace_make_Buffer(&buf, "./tests/testfile");

    ASSERT_EQ(7, Buffer_get_num_lines(&buf));

    Buffer_destroy(&buf);
}

UTEST(Buffer, scroll) {
    Buffer buf;
    ssize_t scroll_amount;
    inplace_make_Buffer(&buf, "./tests/testfile");

    scroll_amount = Buffer_scroll(&buf, 4, 0);
    ASSERT_EQ(0, scroll_amount);
    ASSERT_EQ(0, buf.top_row);
    scroll_amount = Buffer_scroll(&buf, 4, -1);
    ASSERT_EQ(0, scroll_amount);
    ASSERT_EQ(0, buf.top_row);
    scroll_amount = Buffer_scroll(&buf, 4, 1);
    ASSERT_EQ(1, scroll_amount);
    ASSERT_EQ(1, buf.top_row);

    scroll_amount = Buffer_scroll(&buf, 4, 3);
    ASSERT_EQ(2, scroll_amount);
    ASSERT_EQ(3, buf.top_row);

    Buffer_destroy(&buf);
}

