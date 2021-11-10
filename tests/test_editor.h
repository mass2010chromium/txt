#pragma once

#include "../common.h"
#include "../editor/editor.h"
#include "test_utils.h"
#include "editor_private.h"

#define EDITOR_WINDOW_SIZE 4
#define STANDARD_TAB_WIDTH 4

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

UTEST(editor, tab_round_up) {
    TAB_WIDTH = 4;
    for (int i = 0; i < 4; ++i) {
        ASSERT_EQ(4, tab_round_up(i));
    }
    for (int i = 4; i < 8; ++i) {
        ASSERT_EQ(8, tab_round_up(i));
    }
    TAB_WIDTH = 8;
    for (int i = 0; i < 8; ++i) {
        ASSERT_EQ(8, tab_round_up(i));
    }
    TAB_WIDTH = 1;
    for (int i = 0; i < 16; ++i) {
        ASSERT_EQ(i+1, tab_round_up(i));
    }
    TAB_WIDTH = STANDARD_TAB_WIDTH;
}

UTEST(editor, line_pos_ptr) {
    TAB_WIDTH = 4;
    char* line;
    char* ptr;

    line = "Hello\tWorld!";
    ASSERT_EQ(0, line_pos_ptr(line, line));

    ptr = line + 1;
    ASSERT_EQ(1, line_pos_ptr(line, ptr));

    ptr = line + 4;
    ASSERT_EQ(4, line_pos_ptr(line, ptr));

    ptr = line + 5;
    ASSERT_EQ(5, line_pos_ptr(line, ptr));

    ptr = line + 6;
    ASSERT_EQ(8, line_pos_ptr(line, ptr));

    ptr = line + 7;
    ASSERT_EQ(9, line_pos_ptr(line, ptr));

    ptr = line + 12;
    ASSERT_EQ(14, line_pos_ptr(line, ptr));

    line = "Hello\ta\tWorld!";

    ptr = line + 6;
    ASSERT_EQ(8, line_pos_ptr(line, ptr));

    ptr = line + 7;
    ASSERT_EQ(9, line_pos_ptr(line, ptr));

    ptr = line + 8;
    ASSERT_EQ(12, line_pos_ptr(line, ptr));

    ptr = line + 9;
    ASSERT_EQ(13, line_pos_ptr(line, ptr));

    line = "Hello\t\tWorld!";

    ptr = line + 6;
    ASSERT_EQ(8, line_pos_ptr(line, ptr));

    ptr = line + 7;
    ASSERT_EQ(12, line_pos_ptr(line, ptr));

    ptr = line + 8;
    ASSERT_EQ(13, line_pos_ptr(line, ptr));

    TAB_WIDTH = STANDARD_TAB_WIDTH;
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
