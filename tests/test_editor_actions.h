#pragma once

#include "../common.h"
#include "../editor/editor_actions.h"
#include "test_utils.h"
#include "editor_actions_private.h"

UTEST(editor_actions, j_basic) {
    Buffer buf;
    int res;
    inplace_make_Buffer(&buf, "./tests/testfile");
    Buffer* old = current_buffer;
    current_buffer = &buf;

    res = process_action('j', 0, &buf);
    ASSERT_EQ(0, res);
    ASSERT_EQ(0, buf.cursor_col);
    ASSERT_EQ(1, Buffer_get_line_index(&buf, buf.cursor_row));
    
    res = process_action('2', 0, &buf);
    ASSERT_EQ(0, res);
    ASSERT_EQ(0, buf.cursor_col);
    ASSERT_EQ(1, Buffer_get_line_index(&buf, buf.cursor_row));
    res = process_action('j', 0, &buf);
    ASSERT_EQ(0, res);
    ASSERT_EQ(0, buf.cursor_col);
    ASSERT_EQ(3, Buffer_get_line_index(&buf, buf.cursor_row));
    res = process_action('9', 0, &buf);
    ASSERT_EQ(0, res);
    ASSERT_EQ(0, buf.cursor_col);
    ASSERT_EQ(3, Buffer_get_line_index(&buf, buf.cursor_row));
    res = process_action('j', 0, &buf);
    ASSERT_EQ(0, res);
    ASSERT_EQ(0, buf.cursor_col);
    ASSERT_EQ(6, Buffer_get_line_index(&buf, buf.cursor_row));

    res = process_action('j', 0, &buf);
    ASSERT_EQ(0, res);
    ASSERT_EQ(0, buf.cursor_col);
    ASSERT_EQ(6, Buffer_get_line_index(&buf, buf.cursor_row));

    Buffer_destroy(&buf);
    current_buffer = old;
}

UTEST(editor_actions, k_basic) {
    Buffer buf;
    int res;
    inplace_make_Buffer(&buf, "./tests/testfile");
    Buffer* old = current_buffer;
    current_buffer = &buf;
    res = process_action('k', 0, &buf);
    ASSERT_EQ(0, res);
    ASSERT_EQ(0, buf.cursor_col);
    ASSERT_EQ(0, Buffer_get_line_index(&buf, buf.cursor_row));

    res = process_action('9', 0, &buf);
    ASSERT_EQ(0, res);
    res = process_action('j', 0, &buf);
    ASSERT_EQ(0, res);

    res = process_action('k', 0, &buf);
    ASSERT_EQ(0, res);
    ASSERT_EQ(0, buf.cursor_col);
    ASSERT_EQ(5, Buffer_get_line_index(&buf, buf.cursor_row));
    
    res = process_action('2', 0, &buf);
    ASSERT_EQ(0, res);
    ASSERT_EQ(0, buf.cursor_col);
    ASSERT_EQ(5, Buffer_get_line_index(&buf, buf.cursor_row));
    res = process_action('k', 0, &buf);
    ASSERT_EQ(0, res);
    ASSERT_EQ(0, buf.cursor_col);
    ASSERT_EQ(3, Buffer_get_line_index(&buf, buf.cursor_row));

    res = process_action('9', 0, &buf);
    ASSERT_EQ(0, res);
    ASSERT_EQ(0, buf.cursor_col);
    ASSERT_EQ(3, Buffer_get_line_index(&buf, buf.cursor_row));
    res = process_action('k', 0, &buf);
    ASSERT_EQ(0, res);
    ASSERT_EQ(0, buf.cursor_col);
    ASSERT_EQ(0, Buffer_get_line_index(&buf, buf.cursor_row));

    res = process_action('k', 0, &buf);
    ASSERT_EQ(0, res);
    ASSERT_EQ(0, buf.cursor_col);
    ASSERT_EQ(0, Buffer_get_line_index(&buf, buf.cursor_row));

    Buffer_destroy(&buf);
    current_buffer = old;
}

UTEST(editor_actions, l_basic) {
    Buffer buf;
    int res;
    inplace_make_Buffer(&buf, "./tests/testfile");
    Buffer* old = current_buffer;
    current_buffer = &buf;

    res = process_action('l', 0, &buf);
    ASSERT_EQ(0, res);
    ASSERT_EQ(1, buf.cursor_col);
    ASSERT_EQ(0, Buffer_get_line_index(&buf, buf.cursor_row));
    
    res = process_action('2', 0, &buf);
    ASSERT_EQ(0, res);
    ASSERT_EQ(1, buf.cursor_col);
    ASSERT_EQ(0, Buffer_get_line_index(&buf, buf.cursor_row));
    res = process_action('l', 0, &buf);
    ASSERT_EQ(0, res);
    ASSERT_EQ(3, buf.cursor_col);
    ASSERT_EQ(0, Buffer_get_line_index(&buf, buf.cursor_row));
    res = process_action('9', 0, &buf);
    ASSERT_EQ(0, res);
    res = process_action('9', 0, &buf);
    ASSERT_EQ(0, res);
    ASSERT_EQ(3, buf.cursor_col);
    ASSERT_EQ(0, Buffer_get_line_index(&buf, buf.cursor_row));
    res = process_action('l', 0, &buf);
    ASSERT_EQ(0, res);
    ASSERT_EQ(29, buf.cursor_col);
    ASSERT_EQ(0, Buffer_get_line_index(&buf, buf.cursor_row));

    res = process_action('l', 0, &buf);
    ASSERT_EQ(0, res);
    ASSERT_EQ(29, buf.cursor_col);
    ASSERT_EQ(0, Buffer_get_line_index(&buf, buf.cursor_row));

    Buffer_destroy(&buf);
    current_buffer = old;
}

UTEST(editor_actions, h_basic) {
    Buffer buf;
    int res;
    inplace_make_Buffer(&buf, "./tests/testfile");
    Buffer* old = current_buffer;
    current_buffer = &buf;

    res = process_action('h', 0, &buf);
    ASSERT_EQ(0, res);
    ASSERT_EQ(0, buf.cursor_col);
    ASSERT_EQ(0, Buffer_get_line_index(&buf, buf.cursor_row));

    res = process_action('9', 0, &buf);
    ASSERT_EQ(0, res);
    res = process_action('9', 0, &buf);
    ASSERT_EQ(0, res);
    res = process_action('l', 0, &buf);
    ASSERT_EQ(0, res);

    res = process_action('h', 0, &buf);
    ASSERT_EQ(0, res);
    ASSERT_EQ(28, buf.cursor_col);
    ASSERT_EQ(0, Buffer_get_line_index(&buf, buf.cursor_row));

    res = process_action('2', 0, &buf);
    ASSERT_EQ(0, res);
    ASSERT_EQ(28, buf.cursor_col);
    ASSERT_EQ(0, Buffer_get_line_index(&buf, buf.cursor_row));
    res = process_action('h', 0, &buf);
    ASSERT_EQ(0, res);
    ASSERT_EQ(26, buf.cursor_col);
    ASSERT_EQ(0, Buffer_get_line_index(&buf, buf.cursor_row));

    res = process_action('9', 0, &buf);
    ASSERT_EQ(0, res);
    res = process_action('9', 0, &buf);
    ASSERT_EQ(0, res);
    res = process_action('h', 0, &buf);
    ASSERT_EQ(0, res);
    ASSERT_EQ(0, buf.cursor_col);
    ASSERT_EQ(0, Buffer_get_line_index(&buf, buf.cursor_row));

    Buffer_destroy(&buf);
    current_buffer = old;
}

