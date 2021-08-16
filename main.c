#include <termios.h>
#include <unistd.h>
#include <signal.h>

#define DEBUG 1
#ifdef DEBUG
#include <stdio.h>
#include <stdarg.h>
FILE* __of;
void __debug_init() {
    __of = fopen("log.txt", "w");
}
void print(const char* format, ...) {
    va_list ptr;
    va_start(ptr, format);
    vfprintf(__of, format, ptr);
}
#else
void print(const char* format, ...) {}
#endif

void signal_handler(int signum) {
    print("sigint\n");
}

const char BYTE_ESC = 0x1B;

struct termios save_settings;
struct termios set_settings;
int main() {
#ifdef DEBUG
    __debug_init();
#endif
    if (isatty(STDIN_FILENO) == 0) {
        return 1;
    }
    struct sigaction sa;
    sa.sa_handler = signal_handler;
    sigaction(SIGINT, &sa, NULL);
    
    tcgetattr(STDIN_FILENO, &save_settings);
    set_settings = save_settings;
    const tcflag_t local_modes = ISIG | ICANON;
    set_settings.c_cc[VMIN] = 0;
    set_settings.c_cc[VTIME] = 1;
    set_settings.c_lflag = local_modes;
    tcsetattr(STDIN_FILENO, TCSANOW, &set_settings);

    char cls[5];
    cls[0] = BYTE_ESC;
    cls[1] = '[';
    cls[2] = '2';
    cls[3] = 'J';
    cls[4] = 0;
    ssize_t nbytes = write(STDOUT_FILENO, "asdf!", 4);
    nbytes = write(STDOUT_FILENO, cls, 4);
    print("writing cls %d\n", nbytes);
    
    char buf[2];
    buf[1] = '\0';
    for (int i = 0; i < 10; ++i) {
        ssize_t result = read(STDIN_FILENO, buf, 1);
        if (result > 0) {
            print(">> [%x] %s\n", buf[0], buf);
        }
        else {
            print(">] %d\n", result);
        }
        print("[%d]\n", i);
    }

    tcsetattr(STDOUT_FILENO, TCSANOW, &save_settings);
    return 0;
}
