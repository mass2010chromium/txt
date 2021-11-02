#pragma once

#include "../String.h"

UTEST(String, make_String) {
    char* ref = "asdfbsdfcsdfdsdf";
    String* s = make_String(ref);
    ASSERT_EQ(0, strcmp(ref, s->data));
    ASSERT_EQ(strlen(ref), Strlen(s));
    free(s);

    ref = "";
    s = make_String(ref);
    ASSERT_EQ(0, strcmp(ref, s->data));
    ASSERT_EQ(strlen(ref), Strlen(s));
    free(s);
}

UTEST(String, convert_String) {
    char* ref = "asdfbsdfcsdfdsdf";
    String* s = convert_String(strdup(ref));
    ASSERT_EQ(0, strcmp(ref, s->data));
    ASSERT_EQ(strlen(ref), Strlen(s));
    free(s);

    ref = "";
    s = convert_String(strdup(ref));
    ASSERT_EQ(0, strcmp(ref, s->data));
    ASSERT_EQ(strlen(ref), Strlen(s));
    free(s);
}

UTEST(String, Strcats) {
    char* ref = "asdfbsdfcsdfdsdf";
    char* mod = "esdffsdf";
    String* s = make_String(ref);
    String* res = Strcats(&s, mod);
    ASSERT_EQ(s, res);
    ASSERT_EQ(0, strcmp("asdfbsdfcsdfdsdfesdffsdf", s->data));
    ASSERT_EQ(24, Strlen(s));
    free(s);

    ref = "asdfbsdfcsdfdsdf";
    mod = "";
    s = make_String(ref);
    res = Strcats(&s, mod);
    ASSERT_EQ(s, res);
    ASSERT_EQ(0, strcmp("asdfbsdfcsdfdsdf", s->data));
    ASSERT_EQ(16, Strlen(s));
    free(s);

    ref = "";
    mod = "asdfbsdfcsdfdsdf";
    s = make_String(ref);
    res = Strcats(&s, mod);
    ASSERT_EQ(s, res);
    ASSERT_EQ(0, strcmp("asdfbsdfcsdfdsdf", s->data));
    ASSERT_EQ(16, Strlen(s));
    free(s);

    ref = "";
    mod = "";
    s = make_String(ref);
    res = Strcats(&s, mod);
    ASSERT_EQ(s, res);
    ASSERT_EQ(0, strcmp("", s->data));
    ASSERT_EQ(0, Strlen(s));
    free(s);
}

UTEST(String, Strncats) {
    char* ref;
    char* mod;
    String* s;
    String* res;

    ref = "asdfbsdfcsdfdsdf";
    mod = "esdffsdf";
    s = make_String(ref);
    res = Strncats(&s, mod, 8);
    ASSERT_EQ(s, res);
    ASSERT_EQ(0, strcmp("asdfbsdfcsdfdsdfesdffsdf", s->data));
    ASSERT_EQ(24, Strlen(s));
    free(s);

    ref = "asdfbsdfcsdfdsdf";
    mod = "esdffsdf";
    s = make_String(ref);
    res = Strncats(&s, mod, 5);
    ASSERT_EQ(s, res);
    ASSERT_EQ(0, strcmp("asdfbsdfcsdfdsdfesdff", s->data));
    ASSERT_EQ(21, Strlen(s));
    free(s);

    ref = "asdfbsdfcsdfdsdf";
    mod = "";
    s = make_String(ref);
    res = Strncats(&s, mod, 0);
    ASSERT_EQ(s, res);
    ASSERT_EQ(0, strcmp("asdfbsdfcsdfdsdf", s->data));
    ASSERT_EQ(16, Strlen(s));
    free(s);

    ref = "";
    mod = "asdfbsdfcsdfdsdf";
    s = make_String(ref);
    res = Strncats(&s, mod, 8);
    ASSERT_EQ(s, res);
    ASSERT_EQ(0, strcmp("asdfbsdf", s->data));
    ASSERT_EQ(8, Strlen(s));
    free(s);

    ref = "";
    mod = "asdfbsdfcsdfdsdf";
    s = make_String(ref);
    res = Strncats(&s, mod, 0);
    ASSERT_EQ(s, res);
    ASSERT_EQ(0, strcmp("", s->data));
    ASSERT_EQ(0, Strlen(s));
    free(s);

    ref = "";
    mod = "";
    s = make_String(ref);
    res = Strncats(&s, mod, 0);
    ASSERT_EQ(s, res);
    ASSERT_EQ(0, strcmp("", s->data));
    ASSERT_EQ(0, Strlen(s));
    free(s);
}

UTEST(String, Strcat) {
    char* ref = "asdfbsdfcsdfdsdf";
    char* mod = "esdffsdf";
    String* s = make_String(ref);
    String* s2 = make_String(mod);
    String* res = Strcat(&s, s2);
    ASSERT_EQ(s, res);
    ASSERT_EQ(0, strcmp("asdfbsdfcsdfdsdfesdffsdf", s->data));
    ASSERT_EQ(24, Strlen(s));
    ASSERT_EQ(0, strcmp(mod, s2->data));
    free(s);
    free(s2);

    ref = "asdfbsdfcsdfdsdf";
    mod = "";
    s = make_String(ref);
    s2 = make_String(mod);
    res = Strcat(&s, s2);
    ASSERT_EQ(s, res);
    ASSERT_EQ(0, strcmp("asdfbsdfcsdfdsdf", s->data));
    ASSERT_EQ(16, Strlen(s));
    ASSERT_EQ(0, strcmp(mod, s2->data));
    free(s);
    free(s2);

    ref = "";
    mod = "asdfbsdfcsdfdsdf";
    s = make_String(ref);
    s2 = make_String(mod);
    res = Strcat(&s, s2);
    ASSERT_EQ(s, res);
    ASSERT_EQ(0, strcmp("asdfbsdfcsdfdsdf", s->data));
    ASSERT_EQ(16, Strlen(s));
    ASSERT_EQ(0, strcmp(mod, s2->data));
    free(s);
    free(s2);

    ref = "";
    mod = "";
    s = make_String(ref);
    s2 = make_String(mod);
    res = Strcat(&s, s2);
    ASSERT_EQ(s, res);
    ASSERT_EQ(0, strcmp("", s->data));
    ASSERT_EQ(0, Strlen(s));
    ASSERT_EQ(0, strcmp(mod, s2->data));
    free(s);
    free(s2);
}

UTEST(String, String_push) {
    char* ref = "asdf";
    String* s = make_String(ref);
    String_push(&s, 'b');
    ASSERT_EQ(strlen(ref)+1, Strlen(s));
    ASSERT_EQ(0, strcmp("asdfb", s->data));
    free(s);

    ref = "";
    s = make_String(ref);
    String_push(&s, 'm');
    ASSERT_EQ(strlen(ref)+1, Strlen(s));
    ASSERT_EQ(0, strcmp("m", s->data));
    free(s);

    ref = "";
    s = make_String(ref);
    for (int i = 0; i < 10; ++i) {
        String_push(&s, 'm');
    }
    ASSERT_EQ(strlen(ref)+10, Strlen(s));
    ASSERT_EQ(0, strcmp("mmmmmmmmmm", s->data));
    free(s);

    ref = "";
    char* cmp = malloc(257);
    cmp[256] = 0;
    s = make_String(ref);
    for (int i = 0; i < 10; ++i) {
        String_push(&s, i);
        cmp[i] = i;
    }
    ASSERT_EQ(strlen(ref)+10, Strlen(s));
    ASSERT_EQ(0, strcmp(cmp, s->data));
    free(s);
    free(cmp);

    ref = "";
    s = make_String(ref);
    cmp = malloc(1025);
    cmp[1024] = 0;
    memset(cmp, 'm', 1024);
    for (int i = 0; i < 1024; ++i) {
        String_push(&s, 'm');
    }
    ASSERT_EQ(strlen(ref)+1024, Strlen(s));
    ASSERT_EQ(0, strcmp(cmp, s->data));
    free(s);
    free(cmp);
}

UTEST(String, String_delete) {
    char* ref;
    String* s;
    char res;

    ref = "asdfbsdf";
    s = make_String(ref);
    res = String_delete(s, 4);
    ASSERT_EQ('b', res);
    ASSERT_EQ(0, strcmp("asdfsdf", s->data));
    ASSERT_EQ(7, Strlen(s));
    free(s);

    ref = "asdfbsdf";
    s = make_String(ref);
    res = String_delete(s, 0);
    ASSERT_EQ('a', res);
    ASSERT_EQ(0, strcmp("sdfbsdf", s->data));
    ASSERT_EQ(7, Strlen(s));
    free(s);

    ref = "asdfbsdf";
    s = make_String(ref);
    res = String_delete(s, 7);
    ASSERT_EQ('f', res);
    ASSERT_EQ(0, strcmp("asdfbsd", s->data));
    ASSERT_EQ(7, Strlen(s));
    free(s);

    ref = "asdfbsdf";
    s = make_String(ref);
    for (int i = 7; i >= 0; --i) {
        res = String_pop(s);
        ASSERT_EQ(ref[i], res);
    }
    ASSERT_EQ(0, strcmp("", s->data));
    ASSERT_EQ(0, Strlen(s));
    free(s);
}

UTEST(String, String_clear) {
    char* ref = "asdfbsdfcsdfdsdf";
    String* s = make_String(ref);
    String_clear(s);
    ASSERT_EQ(0, strcmp("", s->data));
    ASSERT_EQ(0, Strlen(s));
    free(s);

    ref = "";
    s = make_String(ref);
    ASSERT_EQ(0, strcmp("", s->data));
    ASSERT_EQ(0, Strlen(s));
    free(s);
}

UTEST(String, String_delete_range) {
    char* ref;
    String* s;
    char res;

    ref = "asdf1234";
    s = make_String(ref);
    String_delete_range(s, 2, 6);
    ASSERT_EQ(0, strcmp("as34", s->data));
    ASSERT_EQ(4, Strlen(s));
    free(s);

    ref = "asdf1234";
    s = make_String(ref);
    String_delete_range(s, 0, 2);
    ASSERT_EQ(0, strcmp("df1234", s->data));
    ASSERT_EQ(6, Strlen(s));
    free(s);

    ref = "asdf1234";
    s = make_String(ref);
    String_delete_range(s, 1, 8);
    ASSERT_EQ(0, strcmp("a", s->data));
    ASSERT_EQ(1, Strlen(s));
    free(s);

    ref = "asdfbsdf";
    s = make_String(ref);
    String_delete_range(s, 0, 8);
    ASSERT_EQ(0, strcmp("", s->data));
    ASSERT_EQ(0, Strlen(s));
    free(s);
}

UTEST(String, String_insert) {
    char* ref;
    String* s;

    ref = "asdf1234";
    s = make_String(ref);
    String_insert(&s, 2, '|');
    ASSERT_EQ(0, strcmp("as|df1234", s->data));
    ASSERT_EQ(9, Strlen(s));
    free(s);

    ref = "asdf1234";
    s = make_String(ref);
    String_insert(&s, 0, '|');
    ASSERT_EQ(0, strcmp("|asdf1234", s->data));
    ASSERT_EQ(9, Strlen(s));
    free(s);

    ref = "asdf1234";
    s = make_String(ref);
    String_insert(&s, 8, '|');
    ASSERT_EQ(0, strcmp("asdf1234|", s->data));
    ASSERT_EQ(9, Strlen(s));
    free(s);

    ref = "";
    s = make_String(ref);
    String_insert(&s, 0, '|');
    ASSERT_EQ(0, strcmp("|", s->data));
    ASSERT_EQ(1, Strlen(s));
    free(s);
}

UTEST(String, String_ninserts) {
    char* ref;
    String* s;

    ref = "asdf1234";
    s = make_String(ref);
    String_ninserts(&s, 2, "[|]", 3);
    ASSERT_EQ(0, strcmp("as[|]df1234", s->data));
    ASSERT_EQ(11, Strlen(s));
    free(s);

    ref = "asdf1234";
    s = make_String(ref);
    String_ninserts(&s, 4, "[|]", 1);
    ASSERT_EQ(0, strcmp("asdf[1234", s->data));
    ASSERT_EQ(9, Strlen(s));
    free(s);

    ref = "asdf1234";
    s = make_String(ref);
    String_ninserts(&s, 0, "[|]", 3);
    ASSERT_EQ(0, strcmp("[|]asdf1234", s->data));
    ASSERT_EQ(11, Strlen(s));
    free(s);

    ref = "asdf1234";
    s = make_String(ref);
    String_ninserts(&s, 8, "[|]", 3);
    ASSERT_EQ(0, strcmp("asdf1234[|]", s->data));
    ASSERT_EQ(11, Strlen(s));
    free(s);

    ref = "";
    s = make_String(ref);
    String_ninserts(&s, 0, "|", 1);
    ASSERT_EQ(0, strcmp("|", s->data));
    ASSERT_EQ(1, Strlen(s));
    free(s);

    ref = "";
    s = make_String(ref);
    String_ninserts(&s, 0, "[|]", 3);
    ASSERT_EQ(0, strcmp("[|]", s->data));
    ASSERT_EQ(3, Strlen(s));
    free(s);

    ref = "[|]";
    s = make_String(ref);
    String_ninserts(&s, 2, "asdf1234", 5);
    ASSERT_EQ(0, strcmp("[|asdf1]", s->data));
    ASSERT_EQ(8, Strlen(s));
    free(s);
}