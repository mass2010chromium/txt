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
#define TESTFILE_LEN 7

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
    size_t window_size = 4;

    scroll_amount = Buffer_scroll(&buf, window_size, 0);
    ASSERT_EQ(0, scroll_amount);
    ASSERT_EQ(0, buf.top_row);
    scroll_amount = Buffer_scroll(&buf, window_size, -1);
    ASSERT_EQ(0, scroll_amount);
    ASSERT_EQ(0, buf.top_row);
    scroll_amount = Buffer_scroll(&buf, window_size, 1);
    ASSERT_EQ(1, scroll_amount);
    ASSERT_EQ(1, buf.top_row);

    scroll_amount = Buffer_scroll(&buf, window_size, 3);
    ASSERT_EQ(2, scroll_amount);
    ASSERT_EQ(3, buf.top_row);

    scroll_amount = Buffer_scroll(&buf, window_size, -1);
    ASSERT_EQ(-1, scroll_amount);
    ASSERT_EQ(2, buf.top_row);

    Buffer_destroy(&buf);
}

UTEST(Buffer, get_line) {
    Buffer buf;
    inplace_make_Buffer(&buf, "./tests/testfile");
    size_t window_size = 4;
    ssize_t scroll_delta = 0;
    size_t scroll_amount = 0;

    for (size_t i = 0; i < window_size; ++i) {
        char** linep = Buffer_get_line(&buf, i);
        ASSERT_NE(NULL, linep);
        ASSERT_STREQ(infile_dat[i], *linep);
    }

    scroll_delta = 2;
    scroll_amount += scroll_delta;
    Buffer_scroll(&buf, window_size, scroll_delta);

    for (size_t i = 0; i < window_size; ++i) {
        char** linep = Buffer_get_line(&buf, i);
        ASSERT_NE(NULL, linep);
        ASSERT_STREQ(infile_dat[i+scroll_amount], *linep);
    }

    scroll_delta = -1;
    scroll_amount += scroll_delta;
    Buffer_scroll(&buf, window_size, scroll_delta);

    for (size_t i = 0; i < window_size; ++i) {
        char** linep = Buffer_get_line(&buf, i);
        ASSERT_NE(NULL, linep);
        ASSERT_STREQ(infile_dat[i+scroll_amount], *linep);
    }

    scroll_delta = -3;
    scroll_amount = 0;
    Buffer_scroll(&buf, window_size, scroll_delta);

    for (size_t i = 0; i < window_size; ++i) {
        char** linep = Buffer_get_line(&buf, i);
        ASSERT_NE(NULL, linep);
        ASSERT_STREQ(infile_dat[i+scroll_amount], *linep);
    }

    Buffer_destroy(&buf);
}

UTEST(Buffer, search_char) {
    Buffer* buf = make_Buffer("./tests/dummy.txt");
    EditorContext ctx;

    //Search success
    ctx.jump_col = 0;
    ctx.jump_row = 0;
    int result = Buffer_search_char(buf, &ctx, 'e', true);
    ASSERT_EQ(0, result);
    ASSERT_EQ(1, ctx.jump_col);
    ASSERT_EQ(0, ctx.jump_row);

    //Search fail
    result = Buffer_search_char(buf, &ctx, 'H', true);
    ASSERT_EQ(-1, result);
    ASSERT_EQ(1, ctx.jump_col);
    ASSERT_EQ(0, ctx.jump_row);

    //Search at end of string
    ctx.jump_col = strlen("Hello, World!") - 1;
    Buffer_search_char(buf, &ctx, 'H', true);
    ASSERT_EQ(-1, result);
    ASSERT_EQ(strlen("Hello, World!") - 1, ctx.jump_col);
    ASSERT_EQ(0, ctx.jump_row);

    //Double search failure
    ctx.jump_col = 0;
    result = Buffer_search_char(buf, &ctx, '!', true);
    result = Buffer_search_char(buf, &ctx, '!', true);
    ASSERT_EQ(-1, result);
    ASSERT_EQ(strlen("Hello, World!") - 1, ctx.jump_col);
    ASSERT_EQ(0, ctx.jump_row);

    Buffer_destroy(buf);
    free(buf);
}

UTEST(Buffer, find_str) {
    Buffer* buf = make_Buffer("./tests/multi_line_text.txt");
    EditorContext ctx;
    ctx.jump_row = 3;
    ctx.jump_col = 1;
    //Bool order: (cross_lines, direction)
    //Search forwards on current line
    int result = Buffer_find_str(buf, &ctx, "x", false, true);
    ASSERT_EQ(0, result);
    ASSERT_EQ(3, ctx.jump_row);
    ASSERT_EQ(2, ctx.jump_col);

    //Search backwards on current line
    result = Buffer_find_str(buf, &ctx, "f", false, false);
    ASSERT_EQ(0, result);
    ASSERT_EQ(3, ctx.jump_row);
    ASSERT_EQ(0, ctx.jump_col);

    //Search forwards across lines
    result = Buffer_find_str(buf, &ctx, "zy", true, true);
    ASSERT_EQ(0, result);
    ASSERT_EQ(7, ctx.jump_row);
    ASSERT_EQ(2, ctx.jump_col);

    //Search backwards across lines
    result = Buffer_find_str(buf, &ctx, "quick", true, false);
    ASSERT_EQ(0, result);
    ASSERT_EQ(1, ctx.jump_row);
    ASSERT_EQ(0, ctx.jump_col);

    //Search backwards inline with content present forwards
    result = Buffer_find_str(buf, &ctx, "ick", false, false);
    ASSERT_EQ(-1, result);
    ASSERT_EQ(1, ctx.jump_row);
    ASSERT_EQ(0, ctx.jump_col);

    //Should not cross lines if set to false
    result = Buffer_find_str(buf, &ctx, "brown", false, true);
    ASSERT_EQ(-1, result);
    ASSERT_EQ(1, ctx.jump_row);
    ASSERT_EQ(0, ctx.jump_col);

    Buffer_destroy(buf);
    free(buf);
}
