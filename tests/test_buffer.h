#pragma once

#include "../common.h"
#include "../structures/buffer.h"
#include "test_utils.h"
#include "buffer_private.h"

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
    Vector_clear_free(&expected, 10);
    Vector_destroy(&expected);
}

UTEST(Buffer, create_destroy) {
    Buffer buf;
    Vector expected;

    inplace_make_Buffer(&buf, "./tests/testfile");
    inplace_make_VS(&expected, infile_dat);

    ASSERT_STREQ("./tests/testfile", buf.name->data);
    ASSERT_EQ(0, buf.top_row);
    ASSERT_EQ(0, buf.cursor_row);
    ASSERT_EQ(0, buf.cursor_col);
    ASSERT_VS_EQ(&expected, &buf.lines);
    
    Vector_clear_free(&expected, 10);
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
        String** linep = Buffer_get_line(&buf, i);
        ASSERT_NE(NULL, linep);
        ASSERT_EQ(Buffer_get_line_abs(&buf, i+scroll_amount), linep);
        ASSERT_STREQ(infile_dat[i], (*linep)->data);
    }

    scroll_delta = 2;
    scroll_amount += scroll_delta;
    Buffer_scroll(&buf, window_size, scroll_delta);

    for (size_t i = 0; i < window_size; ++i) {
        String** linep = Buffer_get_line(&buf, i);
        ASSERT_NE(NULL, linep);
        ASSERT_EQ(Buffer_get_line_abs(&buf, i+scroll_amount), linep);
        ASSERT_STREQ(infile_dat[i+scroll_amount], (*linep)->data);
    }

    scroll_delta = -1;
    scroll_amount += scroll_delta;
    Buffer_scroll(&buf, window_size, scroll_delta);

    for (size_t i = 0; i < window_size; ++i) {
        String** linep = Buffer_get_line(&buf, i);
        ASSERT_NE(NULL, linep);
        ASSERT_EQ(Buffer_get_line_abs(&buf, i+scroll_amount), linep);
        ASSERT_STREQ(infile_dat[i+scroll_amount], (*linep)->data);
    }

    scroll_delta = -3;
    scroll_amount = 0;
    Buffer_scroll(&buf, window_size, scroll_delta);

    for (size_t i = 0; i < window_size; ++i) {
        String** linep = Buffer_get_line(&buf, i);
        ASSERT_NE(NULL, linep);
        ASSERT_EQ(Buffer_get_line_abs(&buf, i+scroll_amount), linep);
        ASSERT_STREQ(infile_dat[i+scroll_amount], (*linep)->data);
    }

    Buffer_destroy(&buf);
}

UTEST(Buffer, get_line_index) {
    Buffer buf;
    inplace_make_Buffer(&buf, "./tests/testfile");
    size_t window_size = 4;
    ssize_t scroll_delta = 0;
    size_t scroll_amount = 0;

    for (size_t i = 0; i < window_size; ++i) {
        size_t linep = Buffer_get_line_index(&buf, i);
        ASSERT_EQ(i + scroll_amount, linep);
    }

    scroll_delta = 2;
    scroll_amount += scroll_delta;
    Buffer_scroll(&buf, window_size, scroll_delta);

    for (size_t i = 0; i < window_size; ++i) {
        size_t linep = Buffer_get_line_index(&buf, i);
        ASSERT_EQ(i + scroll_amount, linep);
    }

    scroll_delta = -1;
    scroll_amount += scroll_delta;
    Buffer_scroll(&buf, window_size, scroll_delta);

    for (size_t i = 0; i < window_size; ++i) {
        size_t linep = Buffer_get_line_index(&buf, i);
        ASSERT_EQ(i + scroll_amount, linep);
    }

    scroll_delta = -3;
    scroll_amount = 0;
    Buffer_scroll(&buf, window_size, scroll_delta);

    for (size_t i = 0; i < window_size; ++i) {
        size_t linep = Buffer_get_line_index(&buf, i);
        ASSERT_EQ(i + scroll_amount, linep);
    }

    Buffer_destroy(&buf);
}

UTEST(Buffer, undo_sanity) {
    Buffer buf;
    inplace_make_Buffer(&buf, "./tests/testfile");

    Edit* insert_content = make_Insert(0, 0, 0, make_String("contents"));
    Buffer_push_undo(&buf, insert_content);

    ASSERT_EQ(1, buf.undo_history.deque.size);

    Buffer_destroy(&buf);
}

UTEST(Buffer, undo_insert1) {
    Buffer buf;
    inplace_make_Buffer(&buf, "./tests/testfile");

    Edit* insert_content;
    char** target;

    insert_content = make_Insert(0, 0, -1, make_String("lol contents"));
    // Should delete the first row, ignoring the new contents.
    Buffer_undo_Edit(&buf, insert_content);
    Edit_destroy(insert_content);
    free(insert_content);
    ASSERT_EQ(6, Buffer_get_num_lines(&buf));

    target = infile_dat + 1;
    for (int i = 0; i < 6; ++i) {
        ASSERT_STREQ(target[i], (*Buffer_get_line(&buf, i))->data);
        ASSERT_STREQ(target[i], (*Buffer_get_line_abs(&buf, i))->data);
    }

    Buffer_destroy(&buf);
}

UTEST(Buffer, undo_insert2) {
    Buffer buf;
    inplace_make_Buffer(&buf, "./tests/testfile");

    Edit* insert_content;
    char** target;

    insert_content = make_Insert(0, 1, -1, make_String("lol contents"));
    // Should delete the second row, ignoring the new contents.
    Buffer_undo_Edit(&buf, insert_content);
    Edit_destroy(insert_content);
    free(insert_content);
    ASSERT_EQ(6, Buffer_get_num_lines(&buf));

    target = infile_dat;
    for (int i = 0; i < 1; ++i) {
        ASSERT_STREQ(target[i], (*Buffer_get_line(&buf, i))->data);
        ASSERT_STREQ(target[i], (*Buffer_get_line_abs(&buf, i))->data);
    }
    
    target = infile_dat + 1;
    for (int i = 1; i < 6; ++i) {
        ASSERT_STREQ(target[i], (*Buffer_get_line(&buf, i))->data);
        ASSERT_STREQ(target[i], (*Buffer_get_line_abs(&buf, i))->data);
    }

    Buffer_destroy(&buf);
}

UTEST(Buffer, undo_insert3) {
    Buffer buf;
    inplace_make_Buffer(&buf, "./tests/testfile");

    Edit* insert_content;
    char** target;

    insert_content = make_Insert(0, 6, -1, make_String("lol contents"));
    // Should delete the last row, ignoring the new contents.
    Buffer_undo_Edit(&buf, insert_content);
    Edit_destroy(insert_content);
    free(insert_content);
    ASSERT_EQ(6, Buffer_get_num_lines(&buf));

    target = infile_dat;
    for (int i = 0; i < 6; ++i) {
        ASSERT_STREQ(target[i], (*Buffer_get_line(&buf, i))->data);
        ASSERT_STREQ(target[i], (*Buffer_get_line_abs(&buf, i))->data);
    }

    Buffer_destroy(&buf);
}

UTEST(Buffer, undo_insert4) {
    Buffer buf;
    inplace_make_Buffer(&buf, "./tests/testfile");

    Edit* insert_content;
    char** target;

    insert_content = make_Insert(0, 5, -1, make_String("lol contents"));
    // Should delete the second to row, ignoring the new contents.
    Buffer_undo_Edit(&buf, insert_content);
    Edit_destroy(insert_content);
    free(insert_content);
    ASSERT_EQ(6, Buffer_get_num_lines(&buf));

    target = infile_dat;
    for (int i = 0; i < 5; ++i) {
        ASSERT_STREQ(target[i], (*Buffer_get_line(&buf, i))->data);
        ASSERT_STREQ(target[i], (*Buffer_get_line_abs(&buf, i))->data);
    }
    
    target = infile_dat + 1;
    for (int i = 5; i < 6; ++i) {
        ASSERT_STREQ(target[i], (*Buffer_get_line(&buf, i))->data);
        ASSERT_STREQ(target[i], (*Buffer_get_line_abs(&buf, i))->data);
    }

    Buffer_destroy(&buf);
}

#include "test_buffer_search.h"

UTEST(Buffer, insert_copy_basic) {

}
