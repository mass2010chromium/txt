#pragma once

#include "../gap_buffer.h"
#include <stdio.h>

UTEST(GapBuffer, make_GapBuffer) {
    char* ref = "Hello, world!\0";
    GapBuffer* buf = make_GapBuffer(ref, DEFAULT_GAP_SIZE);
    ASSERT_EQ(0, strncmp(ref, buf->content + buf->gap_end, strlen(ref)));
    ASSERT_EQ(strlen(ref) + DEFAULT_GAP_SIZE, buf->total_size);
    gapBuffer_destroy(buf);
    free(buf);

    ref = "";
    char* gap_str = calloc(DEFAULT_GAP_SIZE, sizeof(char));
    buf = make_GapBuffer(ref, DEFAULT_GAP_SIZE);
    ASSERT_EQ(0, strncmp(gap_str, buf->content, buf->total_size));
    ASSERT_EQ(strlen(ref) + DEFAULT_GAP_SIZE, buf->total_size);
    gapBuffer_destroy(buf);
    free(buf);
    free(gap_str);
}

UTEST(GapBuffer, gapBuffer_insert) {
    char* ref = "Hello, World!\0";
    char* content_str = calloc(DEFAULT_GAP_SIZE, sizeof(char));
    memcpy(content_str, ref, strlen(ref));
    GapBuffer* buf = make_GapBuffer("", DEFAULT_GAP_SIZE);
    gapBuffer_insert(buf, ref);
    ASSERT_EQ(0, strncmp(content_str, buf->content, buf->total_size));
    ASSERT_EQ(DEFAULT_GAP_SIZE, buf->total_size);
    gapBuffer_destroy(buf);
    free(buf);
    free(content_str);
}

UTEST(GapBuffer, gapBuffer_destroy) {
    GapBuffer* buf = make_GapBuffer("", DEFAULT_GAP_SIZE);
    gapBuffer_destroy(buf);
    ASSERT_EQ(0, buf->total_size);
    ASSERT_EQ(NULL, buf->content);
    free(buf);
}

UTEST(GapBuffer, gapBuffer_get_content) {
    char* ref = "Hello, World!\0";
    char* content_str = calloc(DEFAULT_GAP_SIZE, sizeof(char));
    memcpy(content_str, ref, strlen(ref));
    GapBuffer* buf = make_GapBuffer("", DEFAULT_GAP_SIZE);
    gapBuffer_insert(buf, ref);
    ASSERT_EQ(0, strncmp(content_str, buf->content, buf->total_size));
    ASSERT_EQ(DEFAULT_GAP_SIZE, buf->total_size);
    char* temp = gapBuffer_get_content(buf);
    ASSERT_EQ(0, strncmp(ref, temp, strlen(ref)));
    gapBuffer_destroy(buf);
    free(buf);
    free(content_str);
    free(temp);
}

UTEST(GapBuffer, gapBuffer_resize_fail) {
    GapBuffer* buf = make_GapBuffer("", DEFAULT_GAP_SIZE);
    ASSERT_EQ(DEFAULT_GAP_SIZE, buf->gap_size);
    gapBuffer_resize(buf, 0);
    ASSERT_EQ(DEFAULT_GAP_SIZE, buf->gap_size);
    gapBuffer_destroy(buf);
    free(buf);
}

UTEST(GapBuffer, gapBuffer_resize_success) {
    GapBuffer* buf = make_GapBuffer("", DEFAULT_GAP_SIZE);
    ASSERT_EQ(DEFAULT_GAP_SIZE, buf->gap_size);
    gapBuffer_resize(buf, 1);
    ASSERT_EQ(2*DEFAULT_GAP_SIZE, buf->gap_size);
    gapBuffer_destroy(buf);
    free(buf);
}

UTEST(GapBuffer, gapBuffer_move_gap) {
    char* ref = "Hello, World!\0";
    char* content_str = calloc(DEFAULT_GAP_SIZE, sizeof(char));
    memcpy(content_str, ref, strlen(ref));
    GapBuffer* buf = make_GapBuffer("", DEFAULT_GAP_SIZE);
    gapBuffer_insert(buf, ref);
    ASSERT_EQ(0, strncmp(content_str, buf->content, buf->total_size));
    ASSERT_EQ(DEFAULT_GAP_SIZE, buf->total_size);
    gapBuffer_move_gap(buf, -2);
    char* left_side;
    left_side = strndup(buf->content, buf->gap_start);
    ASSERT_EQ(0, strncmp("Hello, Worl", left_side, strlen(ref) - 2));
    char* right_side;
    right_side = strndup(buf->content + buf->gap_end, buf->total_size - buf->gap_end);
    ASSERT_EQ(0, strncmp("d!", right_side, 2));
    gapBuffer_destroy(buf);
    free(left_side);
    free(right_side);
    free(buf);
    free(content_str);
}

UTEST(GapBuffer, gapBuffer_insert_with_resize) {
    char* ref = "Hello, World!\0";
    GapBuffer* buf = make_GapBuffer("", 1);
    gapBuffer_insert(buf, ref);
    char* temp = gapBuffer_get_content(buf);
    ASSERT_EQ(0, strncmp(ref, temp, strlen(ref)));
    ASSERT_EQ(DEFAULT_GAP_SIZE + 1, buf->total_size);
    ASSERT_EQ(DEFAULT_GAP_SIZE + 1 - strlen(ref), buf->gap_size);
    gapBuffer_destroy(buf);
    free(buf);
    free(temp);
}

UTEST(GapBuffer, gapBuffer_delete) {
    char* ref = "Hello, World!\0";
    char* content_str = calloc(DEFAULT_GAP_SIZE, sizeof(char));
    memcpy(content_str, ref, strlen(ref));
    GapBuffer* buf = make_GapBuffer("", DEFAULT_GAP_SIZE);
    gapBuffer_insert(buf, ref);
    ASSERT_EQ(0, strncmp(content_str, buf->content, buf->total_size));
    ASSERT_EQ(DEFAULT_GAP_SIZE, buf->total_size);
    gapBuffer_delete(buf, 3);
    char* temp = gapBuffer_get_content(buf);
    ASSERT_EQ(0, strncmp(ref, temp, strlen(ref) - 3));
    ASSERT_EQ(DEFAULT_GAP_SIZE - strlen(temp), buf->gap_size);
    ASSERT_EQ(strlen(temp), buf->gap_start);
    ASSERT_EQ(DEFAULT_GAP_SIZE, buf->gap_end);
    gapBuffer_destroy(buf);
    free(buf);
    free(temp);
    free(content_str);
}

UTEST(GapBuffer, gapBuffer_delete_extra) {
    char* ref = "Hello, World!\0";
    char* content_str = calloc(DEFAULT_GAP_SIZE, sizeof(char));
    memcpy(content_str, ref, strlen(ref));
    GapBuffer* buf = make_GapBuffer("", DEFAULT_GAP_SIZE);
    gapBuffer_insert(buf, ref);
    ASSERT_EQ(0, strncmp(content_str, buf->content, buf->total_size));
    ASSERT_EQ(DEFAULT_GAP_SIZE, buf->total_size);
    gapBuffer_delete(buf, 999999999);
    char* temp = gapBuffer_get_content(buf);
    ASSERT_EQ(0, strcmp("", temp));
    ASSERT_EQ(DEFAULT_GAP_SIZE, buf->gap_size);
    ASSERT_EQ(0, buf->gap_start);
    ASSERT_EQ(DEFAULT_GAP_SIZE, buf->gap_end);
    gapBuffer_destroy(buf);
    free(buf);
    free(temp);
    free(content_str);
}