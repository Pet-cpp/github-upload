#ifndef CLIENT_NAAS_TUN_H
#define CLIENT_NAAS_TUN_H

int tun_alloc(char *dev, int flags);
void tun_set_ip(char *ip, char *tun_name);
void tun_up(char *tun_name);
void set_route(char *ip, char *tun_name);

#endif //CLIENT_NAAS_TUN_H
