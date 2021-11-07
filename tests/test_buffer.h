#pragma once

#include "../common.h"
#include "../buffer.h"

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