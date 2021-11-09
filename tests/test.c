#include <unistd.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>

#include "utest.h"

#include "test_string.h"
#include "test_buffer.h"
#include "test_gapbuffer.h"
#include "test_editor.h"

UTEST_STATE();

int main(int argc, const char** argv) {
    editor_init("tests/scratchfile");
    utest_main(argc, argv);
}
