
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>

#include <unistd.h>

#include <string.h>
#include <stdlib.h>

#include <iostream>
#include <string>

#include <event2/event.h>

/*
  register:
  chat1: 发起
  chat2: 
 */

class HlpContext 
{
public:
    
};

typedef struct {
    std::string cmd;
    std::string from;
    std::string to;
    std::string value;
} HlpCmd;

void cb_func(evutil_socket_t fd, short what, void *args)
{
    char buff[1024] = {0};
    char ip_addr[32] = {0};
    unsigned short ip_port = 0;
    int iret;
    struct sockaddr_in sa;
    socklen_t slen;

    // iret = recv(fd, buff, 1024, 0);
    iret = recvfrom(fd, buff, 1024, 0, (struct sockaddr*)&sa, &slen);

    inet_ntop(AF_INET, &sa.sin_addr.s_addr, ip_addr, 32);
    ip_port = ntohs(sa.sin_port);

    printf("read iret: %d, %s:%d, %s\n", iret, ip_addr, ip_port, buff);
    
}


int main(int argc, char **argv)
{
    struct event_base *heb = NULL;
    struct event *hev = NULL;
    struct timeval five_seconds = {2, 0};
    int iret;

    // init 
    heb = event_base_new();

    struct sockaddr_in saddr;
    socklen_t slen;

    saddr.sin_family = AF_INET;
    saddr.sin_port = htons(7890);
    // iret = inet_pton(AF_INET, "81.4.106.67", &saddr.sin_addr.s_addr);
    iret = inet_pton(AF_INET, "0.0.0.0", &saddr.sin_addr.s_addr);
    printf("iret: %d, %s\n", iret, strerror(errno));

    int fd = socket(AF_INET, SOCK_DGRAM | SOCK_NONBLOCK, 0);
    printf("iret: %d, %s\n", fd, strerror(errno));

    iret = ::bind(fd, (struct sockaddr*)&saddr, sizeof(struct sockaddr));
    printf("iret: %d, %s\n", iret, strerror(errno));

    hev = event_new(heb, fd, EV_READ | EV_PERSIST, cb_func, heb);
    // event_add(hev, &five_seconds);
    event_add(hev, NULL);

    int eret = event_base_loop(heb, 0);
    printf("eret: %d\n", eret);

    sleep(5);

    return 0;
}
