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
#ifdef DEBUG
    __debug_init();
#endif
    editor_init("tests/scratchfile");
    editor_bottom = EDITOR_WINDOW_SIZE;
    utest_main(argc, argv);
}
