#pragma once

#include "../common.h"
#include "../editor/editor.h"
#include "test_utils.h"

#define EDITOR_WINDOW_SIZE 4

// TODO: Disable screen interaction during editor tests...

UTEST(editor, make_buffer) {
    editor_make_buffer("./tests/scratchfile");
    editor_switch_buffer(1);

    Vector expected;
    inplace_make_VS(&expected, infile_dat);

    ASSERT_STREQ("./tests/scratchfile", current_buffer->name);
    ASSERT_EQ(0, current_buffer->top_row);
    ASSERT_EQ(0, current_buffer->cursor_row);
    ASSERT_EQ(0, current_buffer->cursor_col);
    ASSERT_EQ(EDITOR_WINDOW_SIZE, editor_bottom);
    ASSERT_VS_EQ(&expected, &current_buffer->lines);

    editor_close_buffer(1);
    Vector_destroy(&expected);
}

UTEST(editor, editor_move_down) {
    editor_make_buffer("./tests/scratchfile");
    editor_switch_buffer(1);

    editor_move_down();
    ASSERT_EQ(1, current_buffer->cursor_row);
    ASSERT_EQ(0, current_buffer->cursor_col);

    editor_move_down();
    ASSERT_EQ(2, current_buffer->cursor_row);
    ASSERT_EQ(0, current_buffer->cursor_col);

    editor_move_down();
    ASSERT_EQ(3, current_buffer->cursor_row);
    ASSERT_EQ(0, current_buffer->cursor_col);

    editor_move_down();
    ASSERT_EQ(3, current_buffer->cursor_row);
    ASSERT_EQ(0, current_buffer->cursor_col);

    editor_move_down();
    ASSERT_EQ(3, current_buffer->cursor_row);
    ASSERT_EQ(0, current_buffer->cursor_col);

    editor_close_buffer(1);
}

UTEST(editor, begin_end_insert) {
    // Sanity checks for memory leaks.
    editor_make_buffer("./tests/scratchfile");
    editor_switch_buffer(1);

    begin_insert();
    ASSERT_EQ(EM_INSERT, Buffer_get_mode(current_buffer));

    GapBuffer* gb = &active_insert;
    ASSERT_STREQ("", gb->content);
    ASSERT_STREQ(infile_dat[0], gb->content + gb->gap_end);
    ASSERT_EQ(EDITOR_WINDOW_SIZE, editor_bottom);

    end_insert();

    editor_close_buffer(1);
}