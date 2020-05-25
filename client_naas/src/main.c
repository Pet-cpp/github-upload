#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <net/if.h>
#include <linux/if_tun.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <errno.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include "../include/tun.h"
#include "../include/daemonize.h"
#include "../include/msg.h"
#include "../include/log.h"

#define BUFSIZE 65536
#define PORT 55555


typedef struct {
    unsigned int ip;
    unsigned int ip_client;
    int netmask_send;
    int count_route;
}Send_ip;

void close_connection(int tun_fd, int net_fd) {
    close(tun_fd);
    close(net_fd);
}

int main(int argc, char *argv[]) {

    int tun_fd, option;
    int log;
    const char *log_file_name = "error.log";
    char if_name[IFNAMSIZ] = "";
    char client_ip[19];
    int maxfd;
    uint16_t nread, nwrite;
    char buffer[BUFSIZE];
    struct sockaddr_in remote;
    char remote_ip[19] = "";
    unsigned short int port = PORT;
    int net_fd;
    char net_name[20] = "";
    prog_name = argv[0];

    while((option = getopt(argc, argv, "n:i:c:p:h")) > 0) {
        switch(option) {
            case 'h':
                usage();
                break;
            case 'i':
                strncpy(if_name,optarg, IFNAMSIZ-1);
                break;
            case 'c':
                strncpy(remote_ip, optarg,15);
                break;
            case 'p':
                port = atoi(optarg);
                break;
            case 'n':
                strncpy(net_name, optarg,20);
                break;
            default:
                my_err("Unknown option %c\n", option);
                usage();
        }
    }

    if(*remote_ip == '\0') {
        my_err("Must specify server address!\n");
        usage();
        return -1;
    }

    log = log_open(log_file_name);
    if (!log) {
        my_err("can't open log file\n");
        return -1;
    }

    log_info(log, "%s is started", prog_name);

    if ((tun_fd = tun_alloc(if_name, IFF_TUN | IFF_NO_PI)) < 0 ) {
        log_error(log,"Error connecting to tun interface %s!", if_name);
        return -1;
    }

    if ((net_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        close(tun_fd);
        log_error(log,"Error opening socket");
        return -1;
    }

    memset(&remote, 0, sizeof(remote));
    remote.sin_family = AF_INET;
    remote.sin_addr.s_addr = inet_addr(remote_ip);
    remote.sin_port = htons(port);

    if (connect(net_fd, (struct sockaddr*) &remote, sizeof(remote)) < 0) {
        close_connection(tun_fd, net_fd);
        log_error(log, "Error connecting to the server: ip - %s, port - %d", remote_ip, port);
        return -1;
    }

    if((write(net_fd, net_name, 20)) < 0){
        char error_buf[1024];
        strerror_r(errno, error_buf, 1024);
        log_error(log, "Writing data to the network (net_name): %s", error_buf);
        return -1;
    }

    Send_ip get_ip;
    if((nread = read(net_fd, &get_ip, sizeof(get_ip))) < 0) {
        char error_buf[1024];
        strerror_r(errno, error_buf, 1024);
        log_error(log, "Reading client ip: %s", error_buf);
        return -1;
    }
    struct in_addr ip_client_send;
    ip_client_send.s_addr = get_ip.ip_client;
    inet_ntop(AF_INET, &ip_client_send, client_ip, 19);

    tun_set_ip(client_ip, if_name);
    log_info(log, "Client address: ip - %s", client_ip);
    tun_up(if_name);
    log_info(log, "%s - The tun interface is up", if_name);

    struct in_addr ip_r_send;
    ip_r_send.s_addr = get_ip.ip;
    char ip_net_mask[19] = "";
    inet_ntop(AF_INET, &ip_r_send, ip_net_mask, 19);
    strcat(ip_net_mask, "/");
    char str[5];
    sprintf(str, "%d", get_ip.netmask_send);
    strcat(ip_net_mask, str);

    my_err("can't open log file -- %s\n", ip_net_mask);

    set_route(ip_net_mask, if_name);
    my_err("can't open log file -- %s\n", ip_net_mask);

    log_info(log, "%s dev %s - Route created", client_ip, if_name);

    for (int i = 0; i < get_ip.count_route; i++) {
        char my_net_route[19];
        if((nread = read(net_fd, my_net_route, sizeof(my_net_route))) < 0) {
            char error_buf[1024];
            strerror_r(errno, error_buf, 1024);
            log_error(log, "Reading route: %s", error_buf);
            return -1;
        }
        set_route(my_net_route, if_name);
        log_info(log, "%s dev %s - Route created", my_net_route, if_name);
    }

    //while (1) {};

    if (daemonize() == -1) {
        log_error(log, "Can't daemonize process");
        close_connection(tun_fd, net_fd);
        return -1;
    }

    log_info(log, "%s is daemonized", prog_name);

    if (setup_signals() == -1) {
        log_error(log, "Can't setup signals");
        close_connection(tun_fd, net_fd);
        return -1;
    }

    maxfd = (tun_fd > net_fd)?tun_fd:net_fd;

    while(1) {
        //log_info(log, "%s is whie", prog_name);
        if (process_is_exited) {
            break;
        }

        int ret;
        fd_set rd_set;

        FD_ZERO(&rd_set);
        FD_SET(tun_fd, &rd_set);
        FD_SET(net_fd, &rd_set);

        ret = select(maxfd + 1, &rd_set, NULL, NULL, NULL);

        if (ret < 0 && errno == EINTR){
            continue;
        }

        if (ret < 0) {
            close_connection(tun_fd, net_fd);
            char error_buf[1024];
            strerror_r(errno, error_buf, 1024);
            log_error(log, "select(): %s", error_buf);
            return -1;
        }

        if(FD_ISSET(tun_fd, &rd_set)) {
            if((nread = read(tun_fd, buffer, BUFSIZE)) < 0) {
                char error_buf[1024];
                strerror_r(errno, error_buf, 1024);
                log_error(log, "Reading data from tun interface: %s", error_buf);
                return -1;
            }

            if((write(net_fd, buffer, nread)) < 0){
                char error_buf[1024];
                strerror_r(errno, error_buf, 1024);
                log_error(log, "Writing data to the network: %s", error_buf);
                return -1;
            }

        }

        if(FD_ISSET(net_fd, &rd_set)) {
            if((nread = read(net_fd, buffer, BUFSIZE)) < 0) {
                char error_buf[1024];
                strerror_r(errno, error_buf, 1024);
                log_error(log, "Reading data from network: %s", error_buf);
                return -1;
            }

            if((write(tun_fd, buffer, nread)) < 0){
                char error_buf[1024];
                strerror_r(errno, error_buf, 1024);
                log_error(log, "Writing data to the tun interface: %s", error_buf);
                return -1;
            }
        }
    }
    log_info(log, "%s is stopped", prog_name);
    log_close(log);
    close_connection(tun_fd, net_fd);

    return 0;
}
