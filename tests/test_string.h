#pragma once

#include "../String.h"

UTEST(String, make_String) {
    char* ref = "asdfbsdfcsdfdsdf";
    String* s = make_String(ref);
    ASSERT_EQ(0, strcmp(ref, s->data));
    free(s);
}
