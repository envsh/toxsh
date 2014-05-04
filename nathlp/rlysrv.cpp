
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>

#include <unistd.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>

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
    std::string type; // server, client, relay
    std::string name;
    std::string ip_addr;
    unsigned short ip_port;
    int cli_fd;
    struct event *cli_ev = NULL;
    time_t mtime;
    struct sockaddr tsa;
};

class HlpContext 
{
public:
    int m_tfd;
    std::map<int, HlpPeer*> m_peers2;
    struct event_base *m_evb = NULL;
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


void cb_func_tcp_read(evutil_socket_t fd, short what, void *args)
{
    struct sockaddr_in sa;
    socklen_t slen = sizeof(sa);
    HlpPeer *peer = NULL;
    HlpContext *pctx = (HlpContext*)args;
    int rc;
    char buff[128] = {0};

    peer = pctx->m_peers2[fd];
    assert(peer != NULL);

    rc = read(fd, buff, sizeof(buff));
    printf("tcp read rc=%d, fd=%d, what=%d\n", rc, fd, what);    
    if (rc < 0) {
    } else if (rc == 0) {
        // close
        close(fd);

        printf("remove peer: %d, %s, %s \n", fd, peer->name.c_str(), peer->type.c_str());      
        event_del(peer->cli_ev);
        pctx->m_peers2.erase(fd);
        delete peer;

    } else if (rc > 0) {
        std::string str(buff, rc);
        std::vector<std::string> vals = string_split(str, ';');
        std::cout<<"vals cnt:"<<vals.size()<<std::endl;

        HlpCmd hc = {vals.at(0), vals.at(1), vals.at(2), vals.at(3)};
        std::cout<<vals.at(0)<<vals.at(1)<<vals.at(2)<<vals.at(3)<<std::endl;

        // register;name;type;ip:port
        if (hc.cmd == "register") {
            // if (hc.cmd == "register" && hc.to == "regsrv") {
            peer->name = hc.from;
            peer->type = hc.to;

            std::vector<std::string> host_elems = string_split(hc.value, ':');
            peer->ip_addr = host_elems.at(0);
            peer->ip_port = atoi(host_elems.at(1).c_str());

            // remove old register
            HlpPeer *tpeer = NULL;
            std::map<int, HlpPeer*>::iterator it;
            for (it = pctx->m_peers2.begin(); it != pctx->m_peers2.end(); it++) {
                tpeer = it->second;
                if (tpeer->cli_fd != peer->cli_fd && tpeer->name == peer->name) {
                    break;
                }
                tpeer = NULL;
            }
            if (tpeer) {
                std::cout<<"close old same name register peer:"<<tpeer->cli_fd<<" "<<tpeer->name
                         <<" "<<tpeer->type<<" "<<tpeer->ip_addr<<":"<<tpeer->ip_port<<std::endl;
                event_del(tpeer->cli_ev);
                ::close(tpeer->cli_fd);
                pctx->m_peers2.erase(tpeer->cli_fd);
                delete tpeer;
            }
        }
        
        if (hc.cmd == "register" && hc.to == "regsrv") {
        } else {  // 除了regsrv自注册外，其他的请求都需要转发到regsrv
            std::map<int, HlpPeer*>::iterator it;
            HlpPeer *speer = NULL, *cpeer = NULL;
            
            for (it = pctx->m_peers2.begin(); it != pctx->m_peers2.end(); it++) {
                speer = it->second;
                if (speer->type == "regsrv") {
                    break;
                } 
                speer = NULL;
            }

            if (speer) {
                if (speer->cli_fd == fd) {
                    // 是server的响应，需要转发给某个客户端
                    for (it = pctx->m_peers2.begin(); it != pctx->m_peers2.end(); it++) {
                        cpeer = it->second;
                        if (cpeer->name == hc.to) {
                            break;
                        }
                        cpeer = NULL;
                    }
                    if (cpeer == NULL) {
                        std::cout<<"can not find relay client:"<<hc.to<<std::endl;
                    } else {
                        rc = ::write(cpeer->cli_fd, buff, rc);
                        std::cout<<"relay to client:"<<str<<std::endl;
                    }
                } else {
                    std::cout<<"relayed cmd to server:"<<str<<speer<<std::endl;
                    rc = ::write(speer->cli_fd, buff, rc);
                    std::cout<<"relayed cmd to server:"<<str<<speer<<" "<<rc<<std::endl;
                }
            } else {
                std::cout<<"can not find server to relay:"<<std::endl;
            }
        }
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
    inet_ntop(AF_INET, &sa.sin_addr.s_addr, buff, sizeof(buff));
    printf("fd=%d, what=%d, clifd=%d, ip=%s, port=%d\n", fd, what, cli_fd, buff, ntohs(sa.sin_port));

    if (pctx->m_peers2.count(cli_fd) == 0) {
        peer = new HlpPeer;
        pctx->m_peers2[cli_fd] = peer;
    } else {
        peer = pctx->m_peers2[cli_fd];
        peer->name = peer->type = peer->ip_addr = "";
    }

    peer->cli_fd = cli_fd;
    memcpy(&peer->tsa, &sa, sizeof(sa));

    // 
    peer->cli_ev = event_new(pctx->m_evb, cli_fd, EV_READ | EV_PERSIST, cb_func_tcp_read, pctx);
    event_add(peer->cli_ev, NULL);
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

    // tcp
    int tfd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
    int reuse = 1;
    setsockopt(tfd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));
    saddr.sin_port = htons(7890);
    iret = ::bind(tfd, (struct sockaddr*)&saddr, sizeof(struct sockaddr));

    hctx.m_tfd = tfd;
    hctx.m_evb = heb;

    htev = event_new(heb, tfd, EV_READ | EV_PERSIST, cb_func_tcp, &hctx);
    event_add(htev, NULL);
   
    hctx.m_tevt = htev;    

    // tcp listen
    iret = ::listen(tfd, 10);

    int eret = event_base_loop(heb, 0);
    printf("eret: %d\n", eret);

    sleep(5);

    return 0;
}
