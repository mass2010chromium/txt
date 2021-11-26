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
    ctx.start_row = 0;
    ctx.start_col = 0;
    ctx.jump_row = 0;
    ctx.jump_col = 0;
    int result = Buffer_find_str(&buf, &ctx, "zy", true, true);
    ASSERT_EQ(0, result);
    ASSERT_EQ(7, ctx.jump_row);
    ASSERT_EQ(2, ctx.jump_col);

    ctx.jump_row = 0;
    ctx.jump_col = 0;
    result = Buffer_find_str(&buf, &ctx, "fox", true, true);
    ASSERT_EQ(0, result);
    ASSERT_EQ(3, ctx.jump_row);
    ASSERT_EQ(0, ctx.jump_col);

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

UTEST(Buffer, find_str_at_cursor) {
    Buffer buf;
    inplace_make_Buffer(&buf, "./tests/text0.txt");
    EditorContext ctx;
    ctx.jump_row = 0;
    ctx.jump_col = 4;
    int result = Buffer_find_str(&buf, &ctx, "fish", false, true);
    ASSERT_EQ(0, result);
    ASSERT_EQ(0, ctx.jump_row);
    ASSERT_EQ(13, ctx.jump_col);

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
    //Skip to next word when starting on punct char
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

UTEST(Buffer, skip_word_edgecases_1) {
    Buffer buf;
    EditorContext ctx;
    ctx.start_col = 0;
    ctx.start_row = 0;
    ctx.jump_col = 0;
    ctx.jump_row = 0;
    inplace_make_Buffer(&buf, "./tests/word_edgecases.txt");

    //Skip 'int'
    int result = Buffer_skip_word(&buf, &ctx, false);
    ASSERT_EQ(0, result);
    ASSERT_EQ(3, ctx.jump_col);
    ASSERT_EQ(0, ctx.jump_row);
    // '* '
    result = Buffer_skip_word(&buf, &ctx, false);
    ASSERT_EQ(0, result);
    ASSERT_EQ(5, ctx.jump_col);
    ASSERT_EQ(0, ctx.jump_row);
    // 'x '
    result = Buffer_skip_word(&buf, &ctx, false);
    ASSERT_EQ(0, result);
    ASSERT_EQ(7, ctx.jump_col);
    ASSERT_EQ(0, ctx.jump_row);
    // '= '
    result = Buffer_skip_word(&buf, &ctx, false);
    ASSERT_EQ(0, result);
    ASSERT_EQ(9, ctx.jump_col);
    ASSERT_EQ(0, ctx.jump_row);
    // '0'
    result = Buffer_skip_word(&buf, &ctx, false);
    ASSERT_EQ(0, result);
    ASSERT_EQ(10, ctx.jump_col);
    ASSERT_EQ(0, ctx.jump_row);
    // ';\n'
    result = Buffer_skip_word(&buf, &ctx, false);
    ASSERT_EQ(0, result);
    ASSERT_EQ(0, ctx.jump_col);
    ASSERT_EQ(1, ctx.jump_row);

    Buffer_destroy(&buf);
}

UTEST(Buffer, skip_word_edgecases_2) {
    Buffer buf;
    EditorContext ctx;
    ctx.start_col = 0;
    ctx.start_row = 0;
    ctx.jump_col = 0;
    ctx.jump_row = 1;
    inplace_make_Buffer(&buf, "./tests/word_edgecases.txt");

    // 'int'
    int result = Buffer_skip_word(&buf, &ctx, false);
    ASSERT_EQ(0, result);
    ASSERT_EQ(3, ctx.jump_col);
    ASSERT_EQ(1, ctx.jump_row);
    // '*'
    result = Buffer_skip_word(&buf, &ctx, false);
    ASSERT_EQ(0, result);
    ASSERT_EQ(4, ctx.jump_col);
    ASSERT_EQ(1, ctx.jump_row);
    // 'y'
    result = Buffer_skip_word(&buf, &ctx, false);
    ASSERT_EQ(0, result);
    ASSERT_EQ(5, ctx.jump_col);
    ASSERT_EQ(1, ctx.jump_row);
    // '='
    result = Buffer_skip_word(&buf, &ctx, false);
    ASSERT_EQ(0, result);
    ASSERT_EQ(6, ctx.jump_col);
    ASSERT_EQ(1, ctx.jump_row);
    // '1234\n\n'
    result = Buffer_skip_word(&buf, &ctx, false);
    ASSERT_EQ(0, result);
    ASSERT_EQ(0, ctx.jump_col);
    ASSERT_EQ(3, ctx.jump_row);

    Buffer_destroy(&buf);
}

UTEST(Buffer, skip_word_edgecases_3) {
    Buffer buf;
    EditorContext ctx;
    ctx.start_col = 0;
    ctx.start_row = 0;
    ctx.jump_col = 0;
    ctx.jump_row = 3;
    inplace_make_Buffer(&buf, "./tests/word_edgecases.txt");

    // '**'
    int result = Buffer_skip_word(&buf, &ctx, false);
    ASSERT_EQ(0, result);
    ASSERT_EQ(2, ctx.jump_col);
    ASSERT_EQ(3, ctx.jump_row);
    // 'x '
    result = Buffer_skip_word(&buf, &ctx, false);
    ASSERT_EQ(0, result);
    ASSERT_EQ(4, ctx.jump_col);
    ASSERT_EQ(3, ctx.jump_row);
    // '= '
    result = Buffer_skip_word(&buf, &ctx, false);
    ASSERT_EQ(0, result);
    ASSERT_EQ(6, ctx.jump_col);
    ASSERT_EQ(3, ctx.jump_row);
    // '(-*'
    result = Buffer_skip_word(&buf, &ctx, false);
    ASSERT_EQ(0, result);
    ASSERT_EQ(9, ctx.jump_col);
    ASSERT_EQ(3, ctx.jump_row);
    // '2'
    result = Buffer_skip_word(&buf, &ctx, false);
    ASSERT_EQ(0, result);
    ASSERT_EQ(10, ctx.jump_col);
    ASSERT_EQ(3, ctx.jump_row);
    // ');\n\n'
    result = Buffer_skip_word(&buf, &ctx, false);
    ASSERT_EQ(0, result);
    ASSERT_EQ(0, ctx.jump_col);
    ASSERT_EQ(5, ctx.jump_row);

    Buffer_destroy(&buf);
}

UTEST(Buffer, skip_word_edgecases_4) {
    Buffer buf;
    EditorContext ctx;
    ctx.start_col = 0;
    ctx.start_row = 0;
    ctx.jump_col = 0;
    ctx.jump_row = 5;
    inplace_make_Buffer(&buf, "./tests/word_edgecases.txt");

    // '__interrupt__ '
    int result = Buffer_skip_word(&buf, &ctx, false);
    ASSERT_EQ(0, result);
    ASSERT_EQ(14, ctx.jump_col);
    ASSERT_EQ(5, ctx.jump_row);
    // 'void '
    result = Buffer_skip_word(&buf, &ctx, false);
    ASSERT_EQ(0, result);
    ASSERT_EQ(19, ctx.jump_col);
    ASSERT_EQ(5, ctx.jump_row);
    // '__func_name2__'
    result = Buffer_skip_word(&buf, &ctx, false);
    ASSERT_EQ(0, result);
    ASSERT_EQ(33, ctx.jump_col);
    ASSERT_EQ(5, ctx.jump_row);
    // '('
    result = Buffer_skip_word(&buf, &ctx, false);
    ASSERT_EQ(0, result);
    ASSERT_EQ(34, ctx.jump_col);
    ASSERT_EQ(5, ctx.jump_row);
    // '_typename_2 '
    result = Buffer_skip_word(&buf, &ctx, false);
    ASSERT_EQ(0, result);
    ASSERT_EQ(46, ctx.jump_col);
    ASSERT_EQ(5, ctx.jump_row);
    // 't2'
    result = Buffer_skip_word(&buf, &ctx, false);
    ASSERT_EQ(0, result);
    ASSERT_EQ(48, ctx.jump_col);
    ASSERT_EQ(5, ctx.jump_row);
    // ') '
    result = Buffer_skip_word(&buf, &ctx, false);
    ASSERT_EQ(0, result);
    ASSERT_EQ(50, ctx.jump_col);
    ASSERT_EQ(5, ctx.jump_row);
    // '{};\n'
    result = Buffer_skip_word(&buf, &ctx, false);
    ASSERT_EQ(0, result);
    ASSERT_EQ(0, ctx.jump_col);
    ASSERT_EQ(6, ctx.jump_row);
    // EOF (failure)
    result = Buffer_skip_word(&buf, &ctx, false);
    ASSERT_EQ(-1, result);
    ASSERT_EQ(0, ctx.jump_col);
    ASSERT_EQ(6, ctx.jump_row);

    Buffer_destroy(&buf);
}

UTEST(Buffer, skip_word_multiline) {
    Buffer buf;
    inplace_make_Buffer(&buf, "./tests/multi_line_text.txt");
    EditorContext ctx;
    ctx.jump_row = 0;
    ctx.jump_col = 0;
    int result = Buffer_skip_word(&buf, &ctx, true);
    ASSERT_EQ(0, result);
    ASSERT_EQ(0, ctx.jump_col);
    ASSERT_EQ(1, ctx.jump_row);

    Buffer_destroy(&buf);
}
