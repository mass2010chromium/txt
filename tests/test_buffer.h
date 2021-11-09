#pragma once

#include "../common.h"
#include "../structures/buffer.h"
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

UTEST(Buffer, search_char_forward_success) {
    Buffer buf;
    inplace_make_Buffer(&buf, "./tests/dummy.txt");
    EditorContext ctx;
    ctx.jump_col = 0;
    ctx.jump_row = 0;
    int result = Buffer_search_char(&buf, &ctx, 'e', true);
    ASSERT_EQ(0, result);
    ASSERT_EQ(1, ctx.jump_col);
    ASSERT_EQ(0, ctx.jump_row);

    Buffer_destroy(&buf);
}

UTEST(Buffer, search_char_forward_fail) {
    Buffer buf;
    inplace_make_Buffer(&buf, "./tests/dummy.txt");
    EditorContext ctx;
    ctx.jump_col = 1;
    ctx.jump_row = 0;
    int result = Buffer_search_char(&buf, &ctx, 'H', true);
    ASSERT_EQ(-1, result);
    ASSERT_EQ(1, ctx.jump_col);
    ASSERT_EQ(0, ctx.jump_row);

    Buffer_destroy(&buf);
}

UTEST(Buffer, search_char_backward_success) {
    Buffer buf;
    inplace_make_Buffer(&buf, "./tests/dummy.txt");
    EditorContext ctx;
    ctx.jump_col = strlen("Hello, World!") - 1;
    ctx.jump_row = 0;
    int result = Buffer_search_char(&buf, &ctx, 'H', false);
    ASSERT_EQ(0, result);
    ASSERT_EQ(0, ctx.jump_col);
    ASSERT_EQ(0, ctx.jump_row);

    Buffer_destroy(&buf);
}

UTEST(Buffer, search_char_backward_edge) {
    Buffer buf;
    inplace_make_Buffer(&buf, "./tests/dummy.txt");
    EditorContext ctx;
    ctx.jump_col = 9;
    ctx.jump_row = 0;
    int result = Buffer_search_char(&buf, &ctx, 'l', false);
    ASSERT_EQ(0, result);
    ASSERT_EQ(3, ctx.jump_col);
    ASSERT_EQ(0, ctx.jump_row);

    Buffer_destroy(&buf);
}

UTEST(Buffer, search_char_backward_fail) {
    Buffer buf;
    inplace_make_Buffer(&buf, "./tests/dummy.txt");
    EditorContext ctx;
    ctx.jump_col = strlen("Hello, World!") - 1;
    ctx.jump_row = 0;
    int result = Buffer_search_char(&buf, &ctx, '!', false);
    ASSERT_EQ(-1, result);
    ASSERT_EQ(strlen("Hello, World!") - 1, ctx.jump_col);
    ASSERT_EQ(0, ctx.jump_row);

    Buffer_destroy(&buf);
}
//Bool order: (cross_lines, direction)
UTEST(Buffer, find_str_forward_inline) {
    Buffer buf;
    inplace_make_Buffer(&buf, "./tests/multi_line_text.txt");
    EditorContext ctx;
    ctx.jump_row = 3;
    ctx.jump_col = 1;
    int result = Buffer_find_str(&buf, &ctx, "x", false, true);
    ASSERT_EQ(0, result);
    ASSERT_EQ(3, ctx.jump_row);
    ASSERT_EQ(2, ctx.jump_col);

    Buffer_destroy(&buf);
}
UTEST(Buffer, find_str_backward_inline) {
    Buffer buf;
    inplace_make_Buffer(&buf, "./tests/multi_line_text.txt");
    EditorContext ctx;
    ctx.jump_row = 3;
    ctx.jump_col = 1;
    int result = Buffer_find_str(&buf, &ctx, "f", false, false);
    ASSERT_EQ(0, result);
    ASSERT_EQ(3, ctx.jump_row);
    ASSERT_EQ(0, ctx.jump_col);

    Buffer_destroy(&buf);
}

UTEST(Buffer, find_str_forward_multiline) {
    Buffer buf;
    inplace_make_Buffer(&buf, "./tests/multi_line_text.txt");
    EditorContext ctx;
    ctx.jump_row = 0;
    ctx.jump_col = 0;
    int result = Buffer_find_str(&buf, &ctx, "zy", true, true);
    ASSERT_EQ(0, result);
    ASSERT_EQ(7, ctx.jump_row);
    ASSERT_EQ(2, ctx.jump_col);

    Buffer_destroy(&buf);
}

UTEST(Buffer, find_str_backward_multiline) {
    Buffer buf;
    inplace_make_Buffer(&buf, "./tests/multi_line_text.txt");
    EditorContext ctx;
    ctx.jump_row = 8;
    ctx.jump_col = 0;
    int result = Buffer_find_str(&buf, &ctx, "quick", true, false);
    ASSERT_EQ(0, result);
    ASSERT_EQ(1, ctx.jump_row);
    ASSERT_EQ(0, ctx.jump_col);

    Buffer_destroy(&buf);
}

UTEST(Buffer, find_str_fail) {
    Buffer buf;
    inplace_make_Buffer(&buf, "./tests/multi_line_text.txt");
    EditorContext ctx;
    ctx.jump_row = 1;
    ctx.jump_col = 0;
    int result = Buffer_find_str(&buf, &ctx, "ick", false, false);
    ASSERT_EQ(-1, result);
    ASSERT_EQ(1, ctx.jump_row);
    ASSERT_EQ(0, ctx.jump_col);

    Buffer_destroy(&buf);
}

UTEST(Buffer, find_str_no_crossing) {
    Buffer buf;
    inplace_make_Buffer(&buf, "./tests/multi_line_text.txt");
    EditorContext ctx;
    ctx.jump_row = 0;
    ctx.jump_col = 0;
    int result = Buffer_find_str(&buf, &ctx, "brown", false, true);
    ASSERT_EQ(-1, result);
    ASSERT_EQ(0, ctx.jump_row);
    ASSERT_EQ(0, ctx.jump_col);

    Buffer_destroy(&buf);
}

UTEST(Buffer, skip_word) {
    Buffer buf;
    EditorContext ctx;
    ctx.jump_col = 0;
    ctx.jump_row = 0;
    inplace_make_Buffer(&buf, "./tests/dummy.txt");
    //Skip to next punctuation char
    int result = Buffer_skip_word(&buf, &ctx, false);
    ASSERT_EQ(0, result);
    ASSERT_EQ(5, ctx.jump_col);
    ASSERT_EQ(0, ctx.jump_row);
    //Skip to next word when starting on space char
    result = Buffer_skip_word(&buf, &ctx, false);
    ASSERT_EQ(0, result);
    ASSERT_EQ(7, ctx.jump_col);
    ASSERT_EQ(0, ctx.jump_row);
    //Should skip punctuation when flag is toggled
    result = Buffer_skip_word(&buf, &ctx, true);
    ASSERT_EQ(0, result);
    ASSERT_EQ(14, ctx.jump_col);
    ASSERT_EQ(0, ctx.jump_row);
    ctx.jump_col = 0;
    result = Buffer_skip_word(&buf, &ctx, true);
    ASSERT_EQ(0, result);
    ASSERT_EQ(7, ctx.jump_col);
    ASSERT_EQ(0, ctx.jump_row);

    Buffer_destroy(&buf);
}
