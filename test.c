#include <unistd.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>

#include "buffer.h"
#include "utils.h"

int main() {
    FILE* test_dat = fopen("testfile", "r");
    char buf[1024];
    Vector v;
    inplace_make_Vector(&v, 10);

    size_t total_size = 0;
    total_size = read_file_break_lines(&v, test_dat);
    char* whole_file = malloc(total_size + 1);
    whole_file[total_size] = 0;
    char* copy_ptr = whole_file;
    for (size_t i = 0; i < v.size; ++i) {
        printf("%s\n", (char*) v.elements[i]);
        strcpy(copy_ptr, v.elements[i]);
        copy_ptr += strlen(v.elements[i]);
        free(v.elements[i]);
    }
    Vector_destroy(&v);

    EditorAction nop_whole_file;
    inplace_make_EditorAction(&nop_whole_file, 0, total_size, ED_INSERT);
    nop_whole_file.buf = whole_file;

    EditorAction nop_whole_file2;
    inplace_make_EditorAction(&nop_whole_file2, 0, total_size, ED_FILE);
    nop_whole_file2.file = test_dat;

    EditorAction insert_x;
    inplace_make_EditorAction(&insert_x, 10, 5, ED_INSERT);
    insert_x.buf = "xxxxx";
    EditorAction_add_child(&nop_whole_file, &insert_x);
    EditorAction_add_child(&nop_whole_file2, &insert_x);

    EditorAction delete_x;
    inplace_make_EditorAction(&delete_x, 10, 5, ED_DELETE);
    delete_x.buf = "aaaaa";
    EditorAction_add_child(&nop_whole_file, &delete_x);
    EditorAction_add_child(&nop_whole_file2, &delete_x);

    EditorContext res;
    resolve_index(&nop_whole_file, 12, &res);
    printf("%lu %lu\n", res.action->size, res.index);

    char* out_buf = malloc(200*sizeof(char));
    size_t n_bytes;
    write_actions_string(out_buf, &nop_whole_file, &n_bytes);
    out_buf[n_bytes] = 0;
    printf("%lu\n%s\n", n_bytes, out_buf);

    FILE* out_dat = fopen("outfile", "w");
    write_actions(out_dat, &nop_whole_file2, &n_bytes);
    printf("%lu\n", n_bytes);

    EditorAction_destroy(&nop_whole_file);
    EditorAction_destroy(&nop_whole_file2);
    EditorAction_destroy(&insert_x);
    EditorAction_destroy(&delete_x);
    free(whole_file);
    free(out_buf);
}
