#ifndef CLIENT_NAAS_DAEMONIZE_H
#define CLIENT_NAAS_DAEMONIZE_H

extern int process_is_exited;

int daemonize();
int setup_signals();

#endif //CLIENT_NAAS_DAEMONIZE_H
