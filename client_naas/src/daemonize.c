#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <signal.h>


#include "../include/daemonize.h"

int process_is_exited = 0;

static void sigterm_handler(int sig) {
    process_is_exited = 1;
}

int daemonize() {
    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);

    pid_t p = fork();
    if (p == -1) {
        return -1;
    }

    if (p != 0) {
        exit(0);
    }

    setsid();

    return 0;
}

int setup_signals() {
    if (signal(SIGTERM, sigterm_handler) == SIG_ERR) {
        return -1;
    }
    return 0;
}
