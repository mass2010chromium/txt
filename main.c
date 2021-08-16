#include <termios.h>
#include <unistd.h>
#include <signal.h>
#include <sys/ioctl.h>
#include <stdlib.h>

#define DEBUG 1
#include "debugging.h"
#include "buffer.h"
#include "editor.h"

extern struct winsize window_size;
extern Buffer* current_buffer;
extern Vector/*char*/ command_buffer;
extern Vector/*Buffer* */ buffers;

EditorMode current_mode = EM_NORMAL;

void signal_handler(int signum) {
    if (signum == SIGINT) {
        current_mode = EM_QUIT;
        return;
    }
    else if (signum == SIGWINCH) {
        ioctl(STDOUT_FILENO, TIOCGWINSZ, &window_size);
        return;
    }
}

struct termios save_settings;
struct termios set_settings;

void process_command(char* command) {
    if (strcmp(command, ":q") == 0) {
        current_mode = EM_QUIT;
        return;
    }
}

void process_input(char input, int control) {
    if (input == BYTE_ESC) {
        if (control == BYTE_UPARROW) {
            if (current_buffer->cursor_row > 1) {
                current_buffer->cursor_row -= 1;
            }
        }
        else if (control == BYTE_DOWNARROW) {
            if (current_buffer->cursor_row < window_size.ws_row - 1) {
                current_buffer->cursor_row += 1;
            }
        }
        else if (control == BYTE_LEFTARROW) {
            if (current_buffer->cursor_col > 1) {
                current_buffer->cursor_col -= 1;
                if (active_insert != NULL) insert_x -= 1;
            }
        }
        else if (control == BYTE_RIGHTARROW) {
            if (current_buffer->cursor_col < window_size.ws_col) {
                current_buffer->cursor_col += 1;
                if (active_insert != NULL) insert_x += 1;
            }
        }
        else {
            current_mode = EM_NORMAL;
            // TODO update data structures
            display_bottom_bar("-- NORMAL --", NULL);
        }
        move_cursor(current_buffer->cursor_row, current_buffer->cursor_col);
        return;
    }
    if (current_mode == EM_INSERT) {
        if (input == BYTE_BACKSPACE) {
            del_chr();
        }
        move_cursor(current_buffer->cursor_row, current_buffer->cursor_col);
    }
    else if (current_mode == EM_NORMAL) {
        char** rows_raw = current_buffer->lines.elements;
        if (input == ':') {
            current_mode = EM_COMMAND;
            Vector_clear(&command_buffer, 10);
            Vector_push(&command_buffer, (void*) input);
            display_bottom_bar(":", NULL);
            move_cursor(window_size.ws_row, 2);
            return;
        }
        else if (input == 'h') {
            if (current_buffer->cursor_col > 1) current_buffer->cursor_col -= 1;
            current_buffer->natural_col = current_buffer->cursor_col;
        }
        else if (input == 'j') {
            if (current_buffer->cursor_row < window_size.ws_row - 1) current_buffer->cursor_row += 1;
            size_t col = strlen(rows_raw[current_buffer->cursor_row-1]) - 1;
            if (col > current_buffer->natural_col) col = current_buffer->natural_col;
            current_buffer->cursor_col = col;
        }
        else if (input == 'k') {
            if (current_buffer->cursor_row > 1) current_buffer->cursor_row -= 1;
            size_t col = strlen(rows_raw[current_buffer->cursor_row-1]) - 1;
            if (col > current_buffer->natural_col) col = current_buffer->natural_col;
            current_buffer->cursor_col = col;
        }
        else if (input == 'l') {
            if (current_buffer->cursor_col < window_size.ws_col
                    && current_buffer->cursor_col < strlen(rows_raw[current_buffer->cursor_row-1])-1) {
                current_buffer->cursor_col += 1;
            }
            current_buffer->natural_col = current_buffer->cursor_col;
        }
        else if (input == 'i') {
            current_mode = EM_INSERT;
            begin_insert();
        }
        move_cursor(current_buffer->cursor_row, current_buffer->cursor_col);
    }
    else if (current_mode == EM_COMMAND) {
        char* buf = malloc(sizeof(char) * command_buffer.size + 2);
        for (int i = 0; i < command_buffer.size; ++i) {
            buf[i] = (char) command_buffer.elements[i];
        }
        if (input == BYTE_ENTER) {
            buf[command_buffer.size] = 0;
            process_command(buf);
            Vector_clear(&command_buffer, 10);
            free(buf);
            move_cursor(window_size.ws_row, 1);
            clear_line();
            return;
        }
        else if (input == BYTE_BACKSPACE) {
            command_buffer.size -= 1;
            buf[command_buffer.size] = 0;
        }
        else {
            buf[command_buffer.size] = input;
            Vector_push(&command_buffer, (void*) input);
            buf[command_buffer.size] = 0;
        }
        display_bottom_bar(buf, NULL);
        free(buf);
        move_cursor(window_size.ws_row, command_buffer.size+1);
        return;
    }
    if (current_mode == EM_INSERT) {
        display_bottom_bar("-- INSERT --", NULL);
    }
    else if (current_mode == EM_NORMAL) {
        display_bottom_bar("-- NORMAL --", NULL);
    }
}

int main() {
#ifdef DEBUG
    __debug_init();
#endif
    editor_init("editor.h");
    if (isatty(STDIN_FILENO) == 0 || isatty(STDOUT_FILENO) == 0) {
        return 1;
    }
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &window_size);
    setvbuf(stdin, NULL, _IONBF, 0);
    setvbuf(stdout, NULL, _IONBF, 0);

    struct sigaction sa = {0};
    sa.sa_handler = signal_handler;
    sigaction(SIGINT, &sa, NULL);
    
    tcgetattr(STDIN_FILENO, &save_settings);
    set_settings = save_settings;
    const tcflag_t local_modes = ~(ICANON|ECHO);
    set_settings.c_cc[VMIN] = 0;
    set_settings.c_cc[VTIME] = 1;
    set_settings.c_lflag &= local_modes;
    tcsetattr(STDIN_FILENO, TCSANOW, &set_settings);

    clear_screen();
    
    char buf[30];
    buf[1] = '\0';
    display_current_buffer();
    process_input(BYTE_ESC, 0);
    while (true) {
        ssize_t result = read(STDIN_FILENO, buf, 1);
        if (result > 0) {
            int control = 0;
            if (buf[0] == BYTE_ESC) {
                result = read(STDIN_FILENO, buf+1, 28);
                if (result > 0) {
                    buf[result+1] = '\0';
                    if (strcmp(buf+1, "[A") == 0) {
                        control = BYTE_UPARROW;
                    }
                    else if (strcmp(buf+1, "[B") == 0) {
                        control = BYTE_DOWNARROW;
                    }
                    else if (strcmp(buf+1, "[C") == 0) {
                        control = BYTE_RIGHTARROW;
                    }
                    else if (strcmp(buf+1, "[D") == 0) {
                        control = BYTE_LEFTARROW;
                    }
                }
            }
            //display_current_buffer();
            buf[result] = 0;
            print("input %c %d\n", buf[0], buf[0]);
            process_input(buf[0], control);
        }
        print("current mode: %d\n", current_mode);
        if (current_mode == EM_QUIT) {
            break;
        }
    }

    tcsetattr(STDOUT_FILENO, TCSANOW, &save_settings);
    return 0;
}

