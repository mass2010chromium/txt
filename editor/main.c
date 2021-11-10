#include <termios.h>
#include <unistd.h>
#include <signal.h>
#include <sys/ioctl.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>

#include "debugging.h"
#include "../structures/buffer.h"
#include "editor.h"
#include "editor_actions.h"
#include "../common.h"

const char* EDITOR_MODE_STR[5] = {
    "NORMAL",
    "INSERT",
    "COMMAND",
    "VISUAL",
    "VISUAL LINE",
};

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

String* bottom_bar_info = NULL;

void close_buffer() {
    editor_close_buffer(current_buffer_idx);
    if (buffers.size == 0) {
        current_mode = EM_QUIT;
    }
    else {
        display_current_buffer();
        String_clear(bottom_bar_info);
        Strcats(&bottom_bar_info, "-- Editing: ");
        Strcats(&bottom_bar_info, current_buffer->name);
        Strcats(&bottom_bar_info, " --");
        display_bottom_bar(bottom_bar_info->data, NULL);
    }
}

void save_buffer() {
    Buffer_save(current_buffer);
    String_clear(bottom_bar_info);
    Strcats(&bottom_bar_info, "-- File saved: ");
    Strcats(&bottom_bar_info, current_buffer->name);
    Strcats(&bottom_bar_info, " --");
    display_bottom_bar(bottom_bar_info->data, NULL);
}

void process_command(char* command) {
    if (strcmp(command, ":q") == 0) {
        close_buffer();
        return;
    }
    if (strcmp(command, ":w") == 0) {
        save_buffer();
        return;
    }
    if (strcmp(command, ":wq") == 0 || strcmp(command, ":qw") == 0) {
        save_buffer();
        close_buffer();
        return;
    }
    if (strncmp(command, ":tabnew ", 8) == 0) {
        char* rest = command + 8;
        editor_make_buffer(rest);
        editor_switch_buffer(buffers.size - 1);
        display_current_buffer();
        String_clear(bottom_bar_info);
        Strcats(&bottom_bar_info, "-- Opened file: ");
        Strcats(&bottom_bar_info, current_buffer->name);
        Strcats(&bottom_bar_info, " --");
        display_bottom_bar(bottom_bar_info->data, NULL);
        return;
    }
    char* scan_start = command + 1;
    char* scan_end = NULL;
    errno = 0;
    long int result = strtol(scan_start, &scan_end, 10);
    if (errno == 0 && result >= 0 && *scan_end == 0) {
        editor_move_to(result, 0);
    }
}

/**
 * Get the corresponding action char.
 */
int esc_action(int control) {
    switch(control) {
        case BYTE_UPARROW:
            return 'k';
        case BYTE_DOWNARROW:
            return 'j';
        case BYTE_LEFTARROW:
            return 'h';
        case BYTE_RIGHTARROW:
            return 'l';
        default:
            return BYTE_ESC;
    }
}

void process_input(char input, int control) {
    if (current_mode == EM_INSERT) {
        display_bottom_bar("-- INSERT --", NULL);
        if (input == BYTE_ESC) {
            int res = esc_action(control);
            if (res == -1) {
                display_bottom_bar("ERROR: Unrecognized escape sequence", NULL);
            }
            else {
                switch(res) {
                    case 'k':
                        editor_move_up();
                        break;
                    case 'j':
                        editor_move_down();
                        break;
                    case 'h':
                        editor_move_left();
                        break;
                    case 'l':
                        editor_move_right();
                        break;
                    default:
                        end_insert();
                        current_mode = EM_NORMAL;
                        // TODO update data structures
                        display_bottom_bar("-- NORMAL --", NULL);
                        // Match vim behavior when exiting insert mode.
                        editor_move_left();
                        editor_align_tab();
                }
                move_to_current();
            }
            Buffer_set_mode(current_buffer, EM_NORMAL);
            return;
        }
        if (input == BYTE_BACKSPACE) { editor_backspace(); }
        else { add_chr(input); }
        move_to_current();
    }
    else if (current_mode == EM_NORMAL) {
        if (input == BYTE_ESC) {
            int res = esc_action(control);
            if (res != -1) input = res;
        }
        if (input == ':') {
            current_mode = EM_COMMAND;
            String_clear(command_buffer);
            String_push(&command_buffer, input);
            display_bottom_bar(":", NULL);
            move_cursor(window_size.ws_row-1, 1);
            return;
        }
        else {
            int res = process_action(input, control);
            if (res == -1) {
                clear_action_stack();
            }
        }
        move_to_current();
        String_clear(bottom_bar_info);
        Strcats(&bottom_bar_info, "-- ");
        Strcats(&bottom_bar_info, EDITOR_MODE_STR[Buffer_get_mode(current_buffer)]);
        Strcats(&bottom_bar_info, " --");
        display_bottom_bar(bottom_bar_info->data, format_action_stack());
    }
    else if (current_mode == EM_COMMAND) {
        if (input == BYTE_ESC) {
            int res = esc_action(control);
            if (res == BYTE_ESC) {
                move_cursor(window_size.ws_row-1, 1);
                clear_line();
                current_mode = EM_NORMAL;
                display_bottom_bar("-- NORMAL --", NULL);
                move_to_current();
                String_clear(command_buffer);
            }
            return;
        }
        if (input == BYTE_ENTER) {
            move_cursor(window_size.ws_row-1, 1);
            clear_line();
            current_mode = EM_NORMAL;
            display_bottom_bar("-- NORMAL --", NULL);
            move_to_current();
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
        move_cursor(window_size.ws_row-1, command_buffer->length);
        return;
    }
}

/**
 * Assumes the program is running in a terminal with support for Xterm's alternate screen buffer.
 * Simply outputs the escape code to display the alternate screen to prevent the editor
 * from overwriting the previous terminal content.
 */
void display_altscreen() {
    printf("\033[?1049h\033[H");
}

/**
 * Assumes the program is running in a terminal with support for Xterm's alternate screen buffer.
 * Simply outputs the escape code to hide the alternate screen. This will restore the terminal
 * content to whatever was present before calling display_altscreen.
 */
void hide_altscreen() {
    printf("\033[?1049l");
}

int main(int argc, char** argv) {
#ifdef DEBUG
    __debug_init();
#endif

    if (argc < 2) {
        printf("Usage: %s <file>\n", argv[0]);
        exit(1);
    }

    bottom_bar_info = alloc_String(20);

    // No terminal supports.
    if (isatty(STDIN_FILENO) == 0 || isatty(STDOUT_FILENO) == 0) {
        return 1;
    }
    display_altscreen();
    fcntl(STDIN_FILENO, F_SETFL, fcntl(STDIN_FILENO, F_GETFL) | O_NONBLOCK);
    setvbuf(stdin, NULL, _IONBF, 0);
    setvbuf(stdout, NULL, _IONBF, 0);

    editor_init(argv[1]);
    init_actions();

    struct sigaction sa = {0};
    sa.sa_handler = signal_handler;
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGWINCH, &sa, NULL);
    
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
            buf[result] = 0;
            print("input %c %d\n", buf[0], buf[0]);
            process_input(buf[0], control);
        }
        if (current_mode == EM_QUIT) {
            break;
        }
    }

    tcsetattr(STDOUT_FILENO, TCSANOW, &save_settings);
    hide_altscreen();
    return 0;
}
