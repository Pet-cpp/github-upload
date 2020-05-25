#include <fcntl.h>
#include <stdarg.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>

#include "../include/log.h"

#define MESSAGE_SIZE 4096

static char *log_levels[] = {
        "info",
        "error",
};

int log_open(const char *file_name) {
    int fd = open(file_name, O_CREAT | O_APPEND | O_WRONLY, S_IRUSR | S_IWUSR);
    if (fd == -1) {
        return -1;
    }

    return fd;
}

int log_close(int fd) {
    return close(fd);
}

static int log_message(int fd, log_level_t level, const char *format, va_list args) {
    char message[MESSAGE_SIZE];
    size_t offset = 0;

    time_t t = time(NULL);
    struct tm tm;
    localtime_r(&t, &tm);
    offset += strftime(message + offset, sizeof(message) - offset, "%Y-%m-%d %H:%M:%S %z\t", &tm);
    offset += snprintf(message + offset, sizeof(message) - offset, "[%s]\t%lu\t", log_levels[level], (unsigned long)getpid());
    offset += vsnprintf(message + offset, sizeof(message) - offset, format, args);
    offset += snprintf(message + offset, sizeof(message) - offset, "\n");
    return write(fd, message, offset);
}

int log_info(int fd, const char *format, ...) {
    va_list args;
    va_start(args, format);
    int rc = log_message(fd, LOG_LEVEL_INFO, format, args);
    va_end(args);
    return rc;
}

int log_error(int fd, const char *format, ...) {
    va_list args;
    va_start(args, format);
    int rc = log_message(fd, LOG_LEVEL_ERROR, format, args);
    va_end(args);
    return rc;
}
