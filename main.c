#include <termios.h>
#include <unistd.h>
#include <signal.h>
#include <sys/ioctl.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>

#include "debugging.h"
#include "buffer.h"
#include "editor.h"

extern struct winsize window_size;
extern Buffer* current_buffer;
extern Vector/*Buffer* */ buffers;

void signal_handler(int signum) {
    if (signum == SIGINT) {
        current_mode = EM_QUIT;
        return;
    }
    else if (signum == SIGWINCH) {
        editor_window_size_change();
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
    if (strcmp(command, ":w") == 0) {
        Buffer_save(current_buffer);
        display_bottom_bar("-- File saved --", NULL);
        return;
    }
    else {

    }
}

void process_esc(int control) {
    if (control == BYTE_UPARROW) { editor_move_up(); }
    else if (control == BYTE_DOWNARROW) { editor_move_down(); }
    else if (control == BYTE_LEFTARROW) { editor_move_left(); }
    else if (control == BYTE_RIGHTARROW) { editor_move_right(); }
    else {
        if (current_mode == EM_INSERT) { end_insert(); }
        current_mode = EM_NORMAL;
        // TODO update data structures
        display_bottom_bar("-- NORMAL --", NULL);
    }
    move_cursor(current_buffer->cursor_row, current_buffer->cursor_col);
}

void process_input(char input, int control) {
    if (current_mode == EM_INSERT) {
        display_bottom_bar("-- INSERT --", NULL);
        if (input == BYTE_ESC) {
            process_esc(control);
            if (current_mode == EM_NORMAL) {
                // Match vim behavior when exiting insert mode.
                editor_move_left();
                move_to_current();
            }
            return;
        }
        if (input == BYTE_BACKSPACE) { editor_backspace(); }
        else { add_chr(input); }
        move_to_current();
    }
    else if (current_mode == EM_NORMAL) {
        display_bottom_bar("-- NORMAL --", NULL);
        if (input == BYTE_ESC) {
            process_esc(control);
            return;
        }
        if (input == ':') {
            current_mode = EM_COMMAND;
            String_clear(command_buffer);
            String_push(&command_buffer, input);
            display_bottom_bar(":", NULL);
            move_cursor(window_size.ws_row, 2);
            return;
        }
        else if (input == 'h') { editor_move_left(); }
        else if (input == 'j' || input == BYTE_ENTER) { editor_move_down(); }
        else if (input == 'k') { editor_move_up(); }
        else if (input == 'l') { editor_move_right(); }
        else if (input == 'u') { 
            int res = editor_undo();
            if (res == 0) {
                display_bottom_bar("-- Undo: Nothing to undo", NULL);
            }
        }
        else if (input == 'x') {
            editor_new_action();
            del_chr();
        }
        else if (input == 'i') {
            display_bottom_bar("-- INSERT --", NULL);
            current_mode = EM_INSERT;
            editor_new_action();
            begin_insert();
        }
        else if (input == 'o') {
            display_bottom_bar("-- INSERT --", NULL);
            current_mode = EM_INSERT;
            editor_new_action();
            editor_newline(1, "\n");
        }
        move_cursor(current_buffer->cursor_row, current_buffer->cursor_col);
    }
    else if (current_mode == EM_COMMAND) {
        if (input == BYTE_ESC) {
            process_esc(control);
            return;
        }
        if (input == BYTE_ENTER) {
            move_cursor(window_size.ws_row, 1);
            clear_line();
            current_mode = EM_NORMAL;
            display_bottom_bar("-- NORMAL --", NULL);
            move_cursor(current_buffer->cursor_row, current_buffer->cursor_col);
            process_command(command_buffer->data);
            String_clear(command_buffer);
            return;
        }
        else if (input == BYTE_BACKSPACE) {
            String_pop(command_buffer);
        }
        else {
            String_push(&command_buffer, input);
        }
        display_bottom_bar(command_buffer->data, NULL);
        move_cursor(window_size.ws_row, command_buffer->length+1);
        return;
    }
}

int main(int argc, char** argv) {
#ifdef DEBUG
    __debug_init();
#endif
    if (argc < 2) {
        printf("Usage: %s <file>\n", argv[0]);
        exit(1);
    }
    if (isatty(STDIN_FILENO) == 0 || isatty(STDOUT_FILENO) == 0) {
        return 1;
    }
    fcntl(STDIN_FILENO, F_SETFL, fcntl(STDIN_FILENO, F_GETFL) | O_NONBLOCK);
    setvbuf(stdin, NULL, _IONBF, 0);
    setvbuf(stdout, NULL, _IONBF, 0);

    editor_init(argv[1]);

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
            editor_fix_view();
        }
        print("current mode: %d\n", current_mode);
        if (current_mode == EM_QUIT) {
            break;
        }
    }

    tcsetattr(STDOUT_FILENO, TCSANOW, &save_settings);
    return 0;
}

