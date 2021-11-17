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

UTEST(editor, format_respect_tabspace_init) {
    TAB_WIDTH = 4;
    char* test_str = "\tint*\tx =\t8;";
    char* test_str_newline = "\tint*\tx =\t8;\n";
    // 01234567890ABCDEF0
    //[    int*    x = 8;]
    char* expected_full = "    int*    x = 8;";
    String* s = alloc_String(0);

    for (int i = 0; i < 8; ++i) {
        format_respect_tabspace(&s, test_str, i, strlen(test_str), 0, (size_t) -1);
        ASSERT_STREQ(expected_full+(i % 4), s->data);
        String_clear(s);
        format_respect_tabspace(&s, test_str_newline, i, strlen(test_str_newline), 0, (size_t) -1);
        ASSERT_STREQ(expected_full+(i % 4), s->data);
        String_clear(s);
    }

    char* expected_2char = "    i";
    for (int i = 0; i < 8; ++i) {
        format_respect_tabspace(&s, test_str, i, 2, 0, (size_t) -1);
        ASSERT_STREQ(expected_2char+(i % 4), s->data);
        String_clear(s);
    }

    free(s);
    TAB_WIDTH = STANDARD_TAB_WIDTH;
}

UTEST(editor, format_respect_tabspace_printlimit) {
    TAB_WIDTH = 4;
    const char* test_str = "\tint*\tx =\t8;";
    // 01234567890ABCDEF0
    //[    int*    x = 8;]
    char* expected;
    String* s = alloc_String(0);

    expected = "";
    for (int i = 0; i < 15; ++i) {
        format_respect_tabspace(&s, test_str, i, strlen(test_str), 0, 0);
        ASSERT_STREQ(expected, s->data);
        String_clear(s);
    }

    expected = " int";
    format_respect_tabspace(&s, test_str, 3, 4, 0, 12);
    ASSERT_STREQ(expected, s->data);
    String_clear(s);

    expected = " int";
    format_respect_tabspace(&s, test_str, 2, 4, 3, 12);
    ASSERT_STREQ(expected, s->data);
    String_clear(s);

    expected = " int* ";
    format_respect_tabspace(&s, test_str, 2, 6, 3, 9);
    ASSERT_STREQ(expected, s->data);
    String_clear(s);

    free(s);
    TAB_WIDTH = STANDARD_TAB_WIDTH;
}

UTEST(editor, format_respect_tabspace_exhaustive) {
    TAB_WIDTH = 4;
    const char* test_str = "\tint*\tx =\t8;";
    const char* expected_full = "    int*    x = 8;";
    // 01234567890ABCDEF0
    //[    int*    x = 8;]
    
    String* s = alloc_String(0);
    
    char expected[20];
    int len = strlen(expected_full);
    for (int j = 0; j < len; ++j) {
        for (int i = j; i < len; ++i) {
            strncpy(expected, expected_full + j, i-j);
            expected[i-j] = 0;
            format_respect_tabspace(&s, test_str, 0, strlen(test_str), j, i);
            ASSERT_STREQ(expected, s->data);
            String_clear(s);
        }
    }
    
    free(s);
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
