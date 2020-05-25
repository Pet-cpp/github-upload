#ifndef CLIENT_NAAS_LOG_H
#define CLIENT_NAAS_LOG_H

typedef enum {
    LOG_LEVEL_INFO = 0,
    LOG_LEVEL_ERROR = 1,
} log_level_t;

int log_open(const char *file_name);
int log_close(int fd);
int log_info(int fd, const char *format, ...);
int log_error(int fd, const char *format, ...);

#endif //CLIENT_NAAS_LOG_H
