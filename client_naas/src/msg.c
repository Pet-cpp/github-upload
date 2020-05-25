#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#include "../include/msg.h"

char *prog_name;

void my_err(char *msg, ...) {
    va_list argp;

    va_start(argp, msg);
    vfprintf(stderr, msg, argp);
    va_end(argp);
}

void usage() {
    fprintf(stderr, "Usage:\n");
    fprintf(stderr, "%s -i <ifacename> [-c <serverIP>] [-p <port>]\n", prog_name);
    fprintf(stderr, "%s -h\n", prog_name);
    fprintf(stderr, "\n");
    fprintf(stderr, "-i <ifacename>: Name of interface to use\n");
    fprintf(stderr, "-c <serverIP>: specify server address (-c <serverIP>) (mandatory)\n");
    fprintf(stderr, "-p <port>: port to connect, default 55555\n");
    fprintf(stderr, "-h: prints this help text\n");
    exit(1);
}

