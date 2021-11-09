#pragma once

#include "../common.h"
#include "../editor/editor.h"
#include "test_utils.h"

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
    ASSERT_VS_EQ(&expected, &current_buffer->lines);

    editor_close_buffer(1);
    Vector_destroy(&expected);
}
