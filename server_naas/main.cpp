#include <sys/types.h>
#include <sys/socket.h>
#include <ctime>
#include <netinet/in.h>
#include <cstdio>
#include <unistd.h>
#include <fcntl.h>
#include <algorithm>
#include <set>
#include <cstdlib>
#include <cstdarg>
#include <cmath>

#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <map>
#include <utility>
#include <iostream>
#include <fstream>
#include <vector>

const int BUFSIZE = 65536;
const int PORT = 55555;
const int IP_ADDR_NEXT = 16777216;
char *prog_name;

void usage() {
    fprintf(stderr, "Usage:\n");
    fprintf(stderr, "%s [-c </path/to/config/file>] [-a <serverIP>] [-p <port>]\n", prog_name);
    fprintf(stderr, "%s -h\n", prog_name);
    fprintf(stderr, "\n");
    fprintf(stderr, "-c </path/to/congig/file>: Path to the configuration file (mandatory)\n");
    fprintf(stderr, "-a <serverIP>: specify server address (-c <serverIP>) (mandatory)\n");
    fprintf(stderr, "-p <port>: port to connect, default 55555\n");
    fprintf(stderr, "-h: prints this help text\n");
    exit(1);
}

typedef struct {
   u_char ip_vhl;      /* version << 4 | header length >> 2 */
    u_char ip_tos;      /* type of service */
    u_short ip_len;     /* total length */
    u_short ip_id;      /* identification */
    u_short ip_off;     /* fragment offset field */
#define IP_RF 0x8000        /* reserved fragment flag */
#define IP_DF 0x4000        /* dont fragment flag */
#define IP_MF 0x2000        /* more fragments flag */
#define IP_OFFMASK 0x1fff   /* mask for fragmenting bits */
    u_char ip_ttl;      /* time to live */
    u_char ip_p;        /* protocol */
    u_short ip_sum;     /* checksum */
    struct in_addr ip_src, ip_dst; /* source and dest address */
} IPHeader;

typedef struct {
    IPHeader header;
    char data[65515];
} IPPacket;

class Network {
public:
    explicit Network(const char* vpn_ip_1, std::string  vpn_net_name_1 = "vpn1") :
    vpn_net_name_1(std::move((vpn_net_name_1)))
    {
        inet_aton(vpn_ip_1, &vpn_net_ip_1);
        vpn1.clear();
        last_ip = vpn_net_ip_1;
    }

    int max_fd() {
        return std::max_element(vpn1.begin(), vpn1.end(), [](const std::pair<unsigned int, int> A, const std::pair<unsigned int, int> B) {
            return  A.second < B.second;
        })->second;
    }

    void set_ip(int sock_client) {
        i++;


        in_addr new_ip = last_ip;
        new_ip.s_addr += IP_ADDR_NEXT;
        write(sock_client, &new_ip, sizeof(new_ip));
        last_ip = new_ip;
        if (i==2) {
            std::cout << "new_ip " << last_ip.s_addr << std::endl;
            new_ip.s_addr = 939194967;
        }
        vpn1.insert(std::pair<unsigned int, int>(new_ip.s_addr, sock_client));
    }

    void send_packet(int sock_client, char *buf) {
        IPPacket *packet;
        packet = (IPPacket *)malloc(sizeof(IPPacket));
        memcpy(packet, (char *)&buf, sizeof(IPPacket));
        //printf("dst = %u.%u.%u.%u\n", packet->header.dest & 0xFF, (packet->header.dest & 0xFF00) >> 8, (packet->header.dest & 0xFF0000) >> 16, (packet->header.dest & 0xFF000000) >> 24);

    }

    std::map<unsigned int, int> vpn1;
private:
    int i = 0;
    in_addr vpn_net_ip_1{};
    std::string vpn_net_name_1;
    in_addr last_ip{};

};

void my_err(char *msg, ...) {
    va_list argp;

    va_start(argp, msg);
    vfprintf(stderr, msg, argp);
    va_end(argp);
}

typedef struct {
    unsigned int ip;
    unsigned int ip_client;
    int netmask_send;
    int count_route;
}Send_ip;

typedef struct {
    int sock;
    std::string name;
}Sock_and_name;

typedef struct {
    char address_ip[16];
    unsigned int addr_ip_4b;
    int netmask;
    int count_route;
    std::vector<std::string> net_route;
    in_addr last_ip;
    int count_client;
    std::vector<unsigned int> free_addr;
}My_route;

class Route {
public:
    explicit Route(std::istream& is) {
        std::string net_name;
        while (is >> net_name) {
            My_route temp_rout;
            is >> temp_rout.address_ip;
            is >> temp_rout.netmask;
            is >> temp_rout.count_route;
            for (auto i = 0; i < temp_rout.count_route; i++) {
                std::string temp_net_r;
                is >> temp_net_r;
                temp_rout.net_route.push_back(temp_net_r);
            }
            inet_aton(temp_rout.address_ip, &temp_rout.last_ip);
            temp_rout.addr_ip_4b = temp_rout.last_ip.s_addr;
            temp_rout.count_client = 0;
            temp_rout.free_addr.clear();
            rout.insert(std::make_pair(net_name, temp_rout));
        }
        vpn.clear();
    }

    int new_connection(int sock_client) {
        char buf_net_name[20];
        int bytes_read = read(sock_client,buf_net_name, 20);

        if (bytes_read <= 0) {
            close(sock_client);
            return -1;
        }

        auto it = rout.find(buf_net_name);
        if (it->second.count_client == pow(2, 32 - it->second.netmask) - 2) {
            close(sock_client);
            return -1;
        }

        if (it != rout.end()) {
            Send_ip temp_send_ip;
            temp_send_ip.ip = it->second.addr_ip_4b;
            if (!it->second.free_addr.empty()) {
                temp_send_ip.ip_client = it->second.free_addr[0];
                it->second.free_addr.erase(it->second.free_addr.begin());
            } else {
                temp_send_ip.ip_client = it->second.last_ip.s_addr + IP_ADDR_NEXT;
                it->second.last_ip.s_addr = temp_send_ip.ip_client;
            }

            temp_send_ip.netmask_send = it->second.netmask;
            temp_send_ip.count_route = it->second.count_route;

            if (write(sock_client, &temp_send_ip, sizeof(temp_send_ip)) < 0) {
                close(sock_client);
                return -1;
            }

            for (auto & i : it->second.net_route) {
                char buf_rout[19];
                strcpy(buf_rout, i.c_str());
                if (write(sock_client, buf_rout, sizeof(buf_rout)) < 0) {
                    close(sock_client);
                    return -1;
                }
            }
            it->second.count_client += 1;
            Sock_and_name temp_sock_name;
            temp_sock_name.sock = sock_client;
            temp_sock_name.name = it->first;
            vpn.insert(std::make_pair(temp_send_ip.ip_client, temp_sock_name));
        }
        return 0;
    }

    int max_fd() {
        return std::max_element(vpn.begin(), vpn.end(), [](const std::pair<unsigned int, Sock_and_name> A, const std::pair<unsigned int, Sock_and_name> B) {
            return  A.second.sock < B.second.sock;
        })->second.sock;
    }

    void disconnect(std::string net_name, unsigned int ip) {
        auto it = rout.find(net_name);
        if (it != rout.end()) {
            it->second.count_client -= 1;
            it->second.free_addr.push_back(ip);
        }
    }

    void cout_route() {
        for(auto & it : rout) {
            std::cout << it.first << " " << it.second.address_ip << " "
            << it.second.netmask << " " << it.second.count_route << " ";
            for (int i = 0; i < it.second.count_route; i++) {
                std::cout << it.second.net_route[i] << " ";
            }
            std::cout << std::endl;
        }
    }

    std::map<unsigned int, Sock_and_name> vpn;
private:
    std::map<std::string, My_route> rout;
};

int main(int argc, char *argv[]) {
    int listener, option;
    int backlog = 100;
    struct sockaddr_in addr{};
    char buf[BUFSIZE];
    char remote_ip[19] ="";
    int bytes_read;
    const char *path_to_conf;
    unsigned short int port = PORT;

    prog_name = argv[0];

    while((option = getopt(argc, argv, "a:p:c:h")) > 0) {
        switch(option) {
            case 'c': {
                path_to_conf = optarg;
                break;
            }
            case 'a': {
                strncpy(remote_ip, optarg, 19);
                break;
            }
            case 'p': {
                port = atoi(optarg);
                break;
            }
            case 'h': {
                usage();
                break;
            }
            default:
                my_err("Unknown option %c\n", option);
                usage();
        }
    }

    std::ifstream fp(path_to_conf);
    if (!fp) {
        std::cout << "config reading error" << std::endl;
        return -1;
    }

    Route vpn_net_route(fp);
    //vpn_net.cout_route();
    //while (1) {};

    listener = socket(AF_INET, SOCK_STREAM, 0);
    if (listener < 0) {
        perror("socket");
        exit(1);
    }

    fcntl(listener, F_SETFL, O_NONBLOCK);

    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    if (inet_aton(remote_ip, &addr.sin_addr) == 0) {
        close(listener);
        exit(1);
    }

    if (bind(listener, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("bind");
        exit(1);
    }

    if (listen(listener, backlog) == -1) {
        close(listener);
        exit(1);
    }


    //Network net("8.8.8.0");


    while (true) {
        // Çàïîëíÿåì ìíîæåñòâî ñîêåòîâ
        fd_set readset;
        FD_ZERO(&readset);
        FD_SET(listener, &readset);

        for(auto it = vpn_net_route.vpn.begin(); it != vpn_net_route.vpn.end(); it++) {
            FD_SET(it->second.sock, &readset);
        }

        // Çàäà¸ì òàéìàóò
       // timeval timeout;
        //timeout.tv_sec = 15;
       // timeout.tv_usec = 0;

        // Æä¸ì ñîáûòèÿ â îäíîì èç ñîêåòîâ
        int mx = std::max(listener, vpn_net_route.max_fd());
        int ret = select(mx+1, &readset, NULL, NULL, NULL);

        if (ret < 0 && errno == EINTR) {
            continue;
        }

        if (ret < 0) {
            perror("select");
            exit(3);
        }

        // Îïðåäåëÿåì òèï ñîáûòèÿ è âûïîëíÿåì ñîîòâåòñòâóþùèå äåéñòâèÿ
        if(FD_ISSET(listener, &readset)) {

            // Ïîñòóïèë íîâûé çàïðîñ íà ñîåäèíåíèå, èñïîëüçóåì accept
            int sock = accept(listener, NULL, NULL);
            if(sock < 0) {
                perror("accept");
                exit(3);
            }
            vpn_net_route.new_connection(sock);
            fcntl(sock, F_SETFL, O_NONBLOCK);
        }

        for(auto it = vpn_net_route.vpn.begin(); it != vpn_net_route.vpn.end(); it++) {
            if(FD_ISSET(it->second.sock, &readset)) {
                // Ïîñòóïèëè äàííûå îò êëèåíòà, ÷èòàåì èõ
                bytes_read = read(it->second.sock, buf, BUFSIZE);

                if (bytes_read <= 0) {
                    // Ñîåäèíåíèå ðàçîðâàíî, óäàëÿåì ñîêåò èç ìíîæåñòâà
                    close(it->second.sock);
                    vpn_net_route.disconnect(it->second.name, it->first);
                    vpn_net_route.vpn.erase(it);
                    continue;
                }

                IPPacket *packet;
                packet = (IPPacket *)malloc(sizeof(IPPacket));
                memcpy(packet, (char *)&buf, sizeof(IPPacket));
                printf("dst = %u.%u.%u.%u\n", packet->header.ip_dst.s_addr & 0xFF, (packet->header.ip_dst.s_addr & 0xFF00) >> 8, (packet->header.ip_dst.s_addr & 0xFF0000) >> 16, (packet->header.ip_dst.s_addr & 0xFF000000) >> 24);
                //printf("src = %u.%u.%u.%u\n", packet->header.ip_src.s_addr & 0xFF, (packet->header.ip_src.s_addr & 0xFF00) >> 8, (packet->header.ip_src.s_addr & 0xFF0000) >> 16, (packet->header.ip_src.s_addr & 0xFF000000) >> 24);
                auto it = vpn_net_route.vpn.find(packet->header.ip_dst.s_addr);
                if (it != vpn_net_route.vpn.end()) {
                    // std::cout << it->second << std::endl;
                    write(it->second.sock, buf, bytes_read);
                }
                free(packet);

                // Îòïðàâëÿåì äàííûå îáðàòíî êëèåíòó
                //send(*it, buf, bytes_read, 0);
            }
        }
    }

    return 0;
}