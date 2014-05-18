
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>

#include <unistd.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>

#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <map>

#include <event2/event.h>

// __asm__(".symver realpath,realpath@GLIBC_2.2.5");

/*
  register:
  chat1: 发起
  chat2: 

  punch
 */

class HlpPeer
{
public:
    std::string name;
    std::string ip_addr;
    unsigned short ip_port;
    int cli_fd;
    struct event *cli_ev = NULL;
    time_t mtime;
    struct sockaddr usa;
    struct sockaddr usa2;
    struct sockaddr usa3;
    struct sockaddr tsa;
};

class HlpContext 
{
public:
    int m_ufd;
    int m_ufd2;
    int m_ufd3;
    int m_tfd;
    std::map<std::string, HlpPeer*> m_peers;
    struct event_base *m_evb = NULL;
    struct event *m_uevt = NULL;
    struct event *m_uevt2 = NULL;
    struct event *m_uevt3 = NULL;
    struct event *m_tevt = NULL; 
    std::string curr_addr;
    unsigned short curr_port;
    std::string curr_msg;
    struct sockaddr curr_sa;
};

typedef struct {
    std::string cmd;
    std::string from;
    std::string to;
    std::string value;
} HlpCmd;

std::vector<std::string> string_split(std::string str, char sep)
{
    std::vector<std::string> strings;
    // std::istringstream f("denmark;sweden;india;us");
    std::istringstream f(str);
    std::string s;    
    // while (std::getline(f, s, ';')) {
    while (std::getline(f, s, sep)) {
        // std::cout << s << std::endl;
        strings.push_back(s);
    }
    return strings;
}


int nh_sendto(int fd, std::string str, std::string addr, unsigned short port)
{
    struct sockaddr_in sa;
    int rc;
    
    sa.sin_family = AF_INET;
    sa.sin_port = htons(port);
    inet_pton(AF_INET, addr.c_str(), &sa.sin_addr.s_addr);
    
    rc = ::sendto(fd, str.c_str(), str.length(), 0, (struct sockaddr*)&sa, sizeof(struct sockaddr));
    printf("sendto %s:%d, ret=%d,errno=%d,err=%s\n", addr.c_str(), port, rc, errno, strerror(errno));

    return rc;
}

HlpPeer *find_peer(HlpContext *pctx, struct sockaddr *sa)
{
    HlpPeer *peer = NULL, *tpeer;
    std::string uname;
    struct sockaddr_in *isa = (struct sockaddr_in*)sa;
    struct sockaddr_in *usa;
  
    std::map<std::string, HlpPeer*>::iterator it;

    for (it = pctx->m_peers.begin(); it != pctx->m_peers.end(); it++) {
        uname = it->first;
        tpeer = it->second;
        usa = (struct sockaddr_in*)(&tpeer->usa);
        if (usa->sin_addr.s_addr == isa->sin_addr.s_addr) {
            peer = tpeer;
            break;
        }
    }

    return peer;
    return NULL;
}

HlpPeer *find_peer(HlpContext *pctx, int tfd)
{
    HlpPeer *peer = NULL, *tpeer;
    std::string uname;
  
    std::map<std::string, HlpPeer*>::iterator it;

    for (it = pctx->m_peers.begin(); it != pctx->m_peers.end(); it++) {
        uname = it->first;
        tpeer = it->second;
        if (tfd == tpeer->cli_fd) {
            peer = tpeer;
            break;
        }
    }

    return peer;
    return NULL;
}

void state_processor(HlpCmd *pcmd, HlpContext *pctx)
{
    HlpPeer *to_peer = NULL, *from_peer = NULL;
    struct sockaddr_in sa;
    int rc;
    char buff[128] = {0};

    if (pcmd->cmd == "register1" || pcmd->cmd == "register2" || pcmd->cmd == "register3") {
        if (pctx->m_peers.count(pcmd->from) > 0) {
            from_peer = pctx->m_peers[pcmd->from];
        } else {
            from_peer = new HlpPeer;
            from_peer->name = pcmd->from;
            from_peer->ip_addr = pctx->curr_addr;
            from_peer->ip_port = pctx->curr_port;
        }
        from_peer->mtime = time(NULL);
        pctx->m_peers[pcmd->from] = from_peer;
        
        if (pcmd->cmd == "register1") {
            memcpy(&from_peer->usa, &pctx->curr_sa, sizeof(struct sockaddr_in));
        } else if (pcmd->cmd == "register2") {
            memcpy(&from_peer->usa2, &pctx->curr_sa, sizeof(struct sockaddr_in));
        } else if (pcmd->cmd == "register3") {
            memcpy(&from_peer->usa3, &pctx->curr_sa, sizeof(struct sockaddr_in));
        }
        std::cout<<"curr msg:"<<pctx->curr_msg<<std::endl;

    } else if (pcmd->cmd == "punch_relay") {
        if (pctx->m_peers.count(pcmd->to) > 0) {
            to_peer = pctx->m_peers[pcmd->to];
            from_peer = pctx->m_peers[pcmd->from];
        
            assert(to_peer != NULL);
            assert(from_peer != NULL);
        
            // sendto();
            snprintf(buff, sizeof(buff)-1, "punch_info;%s;%s;%s:%d",
                     pcmd->from.c_str(), pcmd->to.c_str(), pctx->curr_addr.c_str(), pctx->curr_port);
            char *resp_str = buff; // "punch_info;from;to;ip:port";
            rc = write(to_peer->cli_fd, resp_str, strlen(resp_str));
            
            memset(buff, 0, sizeof(buff));
            snprintf(buff, sizeof(buff)-1, "punch_info;%s;%s;%s:%d",
                     pcmd->from.c_str(), pcmd->to.c_str(), to_peer->ip_addr.c_str(), to_peer->ip_port);
            rc = write(from_peer->cli_fd, resp_str, strlen(resp_str));
        } else {
            std::cout<<"error to peer not found:"<<pcmd->to<<std::endl;
        }
        
    } else if (pcmd->cmd == "punch_ok") {
        // punch oked, 
    } else {
        std::cout<<"error unknown cmd:"<<pcmd->cmd<<std::endl;
    }
}

void cb_func(evutil_socket_t fd, short what, void *args)
{
    char buff[1024] = {0};
    char ip_addr[32] = {0};
    unsigned short ip_port = 0;
    int iret;
    struct sockaddr_in sa;
    socklen_t slen = sizeof(sa);

    // iret = recv(fd, buff, 1024, 0);
    iret = recvfrom(fd, buff, 1024, 0, (struct sockaddr*)&sa, &slen);
    if (iret < 0) {
        printf("read err: %d, %s\n", errno, strerror(errno));
    }


    inet_ntop(AF_INET, &sa.sin_addr.s_addr, ip_addr, 32);
    ip_port = ntohs(sa.sin_port);

    printf("read iret: %d, %s:%d, %s\n", iret, ip_addr, ip_port, buff);

    iret = ::sendto(fd, buff, iret, 0, (struct sockaddr*)&sa, sizeof(struct sockaddr));
    printf("echo back: %d, error=%s\n", iret, strerror(errno));

    std::string str(buff);
    std::vector<std::string> vals = string_split(str, ';');
    std::cout<<"vals cnt:"<<vals.size()<<std::endl;

    HlpCmd hc = {vals.at(0), vals.at(1), vals.at(2), vals.at(3)};
    std::cout<<vals.at(0)<<vals.at(1)<<vals.at(2)<<vals.at(3)<<std::endl;

    HlpContext *ctx = (HlpContext*)args;
    ctx->curr_addr = std::string(ip_addr);
    ctx->curr_port = ip_port;
    ctx->curr_msg = str;
    memcpy(&ctx->curr_sa, &sa, sizeof(sa));

    state_processor(&hc, ctx);

    std::map<std::string, std::string> m_peers;

    if (m_peers.count(hc.to) > 0) {
    } else {
        // std::cout<<"to peer not found:"<<hc.to<<std::endl;
    }

}

void cb_func_tcp_read(evutil_socket_t fd, short what, void *args)
{
    struct sockaddr_in sa;
    socklen_t slen = sizeof(sa);
    HlpPeer *peer = NULL;
    HlpContext *pctx = (HlpContext*)args;
    int rc;
    char buff[16] = {0};

    rc = read(fd, buff, sizeof(buff));
    printf("tcp read rc=%d, fd=%d, what=%d\n", rc, fd, what);    
    if (rc == 0) {
        // close
        close(fd);

        peer = find_peer(pctx, fd);
        assert(peer != NULL);
      
        event_del(peer->cli_ev);
        peer->cli_ev = NULL;
        peer->cli_fd = -1;
    }
}

void cb_func_tcp(evutil_socket_t fd, short what, void *args)
{
    struct sockaddr_in sa;
    socklen_t slen = sizeof(sa);
    HlpPeer *peer = NULL;
    HlpContext *pctx = (HlpContext*)args;
    char buff[128] = {0};

    printf("fd=%d, what=%d\n", fd, what);
    int cli_fd = ::accept(fd, (struct sockaddr*)&sa, &slen);
    printf("fd=%d, what=%d, clifd=%d\n", fd, what, cli_fd);

    inet_ntop(AF_INET, &sa.sin_addr.s_addr, buff, sizeof(buff));

    peer = find_peer(pctx, (struct sockaddr*)&sa);
    if (peer == NULL) {
        printf("can not find tcp conn's peer data, %s\n", buff);
    } else {
        peer->cli_fd = cli_fd;
        memcpy(&peer->tsa, &sa, sizeof(sa));

        // 
        peer->cli_ev = event_new(pctx->m_evb, cli_fd, EV_READ | EV_PERSIST, cb_func_tcp_read, pctx);
        event_add(peer->cli_ev, NULL);
    }
}

int main(int argc, char **argv)
{
    struct event_base *heb = NULL;
    struct event *hev = NULL, *hev2 = NULL, *hev3 = NULL, *htev = NULL;
    struct timeval five_seconds = {2, 0};
    int iret;
    HlpContext hctx;

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

    int fd2 = socket(AF_INET, SOCK_DGRAM | SOCK_NONBLOCK, 0);
    saddr.sin_port = htons(7891);
    iret = ::bind(fd2, (struct sockaddr*)&saddr, sizeof(struct sockaddr));

    int fd3 = socket(AF_INET, SOCK_DGRAM | SOCK_NONBLOCK, 0);
    saddr.sin_port = htons(7892);
    iret = ::bind(fd3, (struct sockaddr*)&saddr, sizeof(struct sockaddr));

    // tcp
    int tfd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
    int reuse = 1;
    setsockopt(tfd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));
    saddr.sin_port = htons(7890);
    iret = ::bind(tfd, (struct sockaddr*)&saddr, sizeof(struct sockaddr));

    hctx.m_ufd = fd;
    hctx.m_ufd2 = fd2;
    hctx.m_ufd3 = fd3;
    hctx.m_tfd = tfd;
    hctx.m_evb = heb;

    hev = event_new(heb, fd, EV_READ | EV_PERSIST, cb_func, &hctx);
    // event_add(hev, &five_seconds);
    event_add(hev, NULL);

    hev2 = event_new(heb, fd2, EV_READ | EV_PERSIST, cb_func, &hctx);
    event_add(hev2, NULL);

    hev3 = event_new(heb, fd3, EV_READ | EV_PERSIST, cb_func, &hctx);
    event_add(hev3, NULL);

    htev = event_new(heb, tfd, EV_READ | EV_PERSIST, cb_func_tcp, &hctx);
    event_add(htev, NULL);
   
    hctx.m_uevt = hev;    
    hctx.m_uevt2 = hev2;    
    hctx.m_uevt3 = hev3;    
    hctx.m_tevt = htev;    

    // tcp listen
    iret = ::listen(tfd, 10);

    int eret = event_base_loop(heb, 0);
    printf("eret: %d\n", eret);

    sleep(5);

    return 0;
}
