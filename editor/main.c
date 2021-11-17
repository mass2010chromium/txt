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
    buf[0] = 0;
    char read_buf[15];
    buf[1] = '\0';
    display_current_buffer();
    move_to_current();
    process_input(BYTE_ESC, 0);
    while (true) {
        if (strlen(buf) < 15) {
            ssize_t result = read(STDIN_FILENO, read_buf, 14);
            if (result > 0) {
                read_buf[result] = 0;
                strcat(buf, read_buf);
            }
            else {
                usleep(100);
            }
        }
        if (strlen(buf) > 0) {
            int control = 0;
            int consume = 1;
            if (buf[0] == BYTE_ESC) {
                if (strcmp(buf+1, "[A") == 0) {
                    consume = 3;
                    control = BYTE_UPARROW;
                }
                else if (strcmp(buf+1, "[B") == 0) {
                    consume = 3;
                    control = BYTE_DOWNARROW;
                }
                else if (strcmp(buf+1, "[C") == 0) {
                    consume = 3;
                    control = BYTE_RIGHTARROW;
                }
                else if (strcmp(buf+1, "[D") == 0) {
                    consume = 3;
                    control = BYTE_LEFTARROW;
                }
                else if (strcmp(buf+1, "[3~") == 0) {
                    consume = 4;
                    control = CODE_DELETE;
                }
            }
            print("input %c %d\n", buf[0], buf[0]);
            process_input(buf[0], control);
            memmove(buf, buf+consume, strlen(buf) - consume + 1);
        }
        if (current_mode == EM_QUIT) {
            break;
        }
    }

    tcsetattr(STDOUT_FILENO, TCSANOW, &save_settings);
    hide_altscreen();
    return 0;
}
