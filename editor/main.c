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

struct termios save_settings;
struct termios set_settings = {0};
struct sigaction sa = {0};
struct sigaction sa_old = {0};

volatile bool force_repaint = false;

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

void signal_handler(int signum) {
    sigset_t mask;
    switch (signum) {
        case SIGINT:
            current_mode = EM_QUIT;
            return;
        case SIGWINCH:
            editor_window_size_change();
            force_repaint = true;
            return;
        case SIGTSTP:
            print("sigstop\n");
            tcsetattr(STDIN_FILENO, TCSANOW, &save_settings);
            hide_altscreen();
            sigemptyset(&mask);
            sigaddset(&mask, SIGTSTP);
            sigprocmask(SIG_UNBLOCK, &mask, NULL);
            sigaction(SIGTSTP, &sa_old, NULL);
            kill(0, SIGTSTP);
            sigaction(SIGTSTP, &sa, NULL);
        case SIGCONT:
            print("sigcont\n");
            display_altscreen();

            int n = 10;
            // A signal may cause tcsetattr() to fail (e.g., SIGCONT).
            // Retry a few times. (Source: Vim)
            while (tcsetattr(STDIN_FILENO, TCSANOW, &set_settings) == -1
                    && errno == EINTR && n > 0) {
                --n;
            }

            force_repaint = true;
            return;
    }
}

int main(int argc, char** argv) {
    __debug_init();

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

    sa.sa_handler = signal_handler;
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGWINCH, &sa, NULL);
    sigaction(SIGTSTP, &sa, &sa_old);
    sigaction(SIGCONT, &sa, NULL);
    
    tcgetattr(STDIN_FILENO, &save_settings);
    set_settings = save_settings;
    const tcflag_t local_modes = ~(ICANON | ECHO | ISIG);
    set_settings.c_cc[VMIN] = 1;
    set_settings.c_cc[VTIME] = 0;
    //set_settings.c_iflag &= ~(ICRNL | IXON);
    set_settings.c_iflag &= ~(IXON);
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
            switch(buf[0]) {
                case BYTE_ESC:
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
                    else {
                        print("??? %s", buf+1);
                    }
                    break;
                case BYTE_CTRLC:
                    // TODO: handle
                    kill(0, SIGINT);
                    break;
                case BYTE_CTRLD:
                    // probably do nothing.
                    break;
                case BYTE_CTRLZ:
                    // TODO: handle custom maybe.
                    kill(0, SIGTSTP);
                    break;
            }
            print("input %c %d\n", buf[0], buf[0]);
            process_input(buf[0], control);
            memmove(buf, buf+consume, strlen(buf) - consume + 1);
        }
        if (current_mode == EM_QUIT) {
            print("Quit");
            break;
        }
        if (force_repaint) {
            display_current_buffer();
            force_repaint = false;
        }
    }

    tcsetattr(STDOUT_FILENO, TCSANOW, &save_settings);
    hide_altscreen();
    return 0;
}
