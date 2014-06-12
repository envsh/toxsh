#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <arpa/nameser_compat.h>

#include "ares_dns.h"
#include "ares.h"
#include "kdns.h"

#include <ldns/ldns.h>

// 这个程序使用的上游DNS服务器地址, dnsmasq
#define UPS_DNS_SERVER_ADDR "::1"
#define UPS_DNS_SERVER_PORT 5353

/*
  TODO 内存漏泄，1周涨到400M。
  测试中。。。
 */
KDNS::KDNS()
    : QObject()
{
}

KDNS::~KDNS()
{
}

void KDNS::init()
{
    m_fwd_sock = new QUdpSocket();
    QObject::connect(m_fwd_sock, &QUdpSocket::readyRead, this, &KDNS::onFwdReadyRead);
    m_fwd_sock->connectToHost(UPS_DNS_SERVER_ADDR, UPS_DNS_SERVER_PORT);

    m_sock = new QUdpSocket();
    // m_sock->bind(QHostAddress::AnyIPv6, m_port);
    m_sock->bind(QHostAddress::Any, m_port);

    QObject::connect(m_sock, &QUdpSocket::readyRead, this, &KDNS::onReadyRead);


}

void KDNS::onReadyRead()
{
    qDebug()<<""<<sizeof(HEADER);
    
    QHostAddress addr;
    quint16 port;
    char data[512];
    qint64 iret;
    size_t ilen;
    char *ptr;
    ldns_pkt *qpkt = NULL;
    ldns_rr_list *rr_list = NULL;
    ldns_rr *rr = NULL;

    int cnter = 0;
    while (m_sock->hasPendingDatagrams()) {
        cnter ++;
        Q_ASSERT(m_sock->pendingDatagramSize() < sizeof(data));
        iret = m_sock->readDatagram(data, sizeof(data), &addr, &port);
        if (iret == -1) {
            qDebug()<<"error:"<<m_sock->errorString();
        } else {
            this->debugPacket(data, iret);
            this->processQuery(data, iret, addr, port);
        }
    }
    qDebug()<<"read pkt count:"<<cnter;
}

void KDNS::onFwdReadyRead()
{
    qDebug()<<"";
    
    QHostAddress addr;
    quint16 port;
    char data[512];
    qint64 iret, irc;
    char *ptr;
    int naddrttls;
    char *p;

    struct hostent *host = NULL;
    struct hostent *phost = NULL;

    int cnter = 0;
    while (m_fwd_sock->hasPendingDatagrams()) {
        cnter ++;
        Q_ASSERT(m_fwd_sock->pendingDatagramSize() < sizeof(data));
        iret = m_fwd_sock->readDatagram(data, sizeof(data), &addr, &port);
        if (iret == -1) {
            qDebug()<<"error:"<<m_fwd_sock->errorString();
        } else {
            this->debugPacket(data, iret);
            this->processResponse(data, iret, addr, port);
        }
    }
    qDebug()<<"read pkt count:"<<cnter;
}


void KDNS::processQuery(char *data, int len, QHostAddress host, quint16 port)
{
    ldns_pkt *t_pkt = NULL, *t_pkt2 = NULL;
    ldns_rr_list *t_rr_list = NULL;
    ldns_rr *t_rr = NULL;
    ldns_status t_status;
    char t_buff[256];
    char *t_ptr = NULL;
    size_t t_len;
    QueueItem *qit = new QueueItem();

    qit->m_addr = host;
    qit->m_port = port;
    qit->m_query = QByteArray(data, len);
    qit->m_time = QDateTime::currentDateTime();
    
    t_status = ldns_wire2pkt(&t_pkt, (const uint8_t*)data, len);
    qit->m_qid = ldns_pkt_id(t_pkt);

    QString h = qit->hash();
    if (m_qrqueue.contains(h)) {
        qDebug()<<"what problem???";
    } else {
        m_qrqueue[h] = qit;
        qit = NULL;
    }

    t_rr_list = ldns_pkt_question(t_pkt);
    switch (ldns_rr_type t = ldns_rr_list_type(t_rr_list)) {
    case LDNS_RR_TYPE_A:
        // need IPv6 query
        t_rr = ldns_rr_list_rr(t_rr_list, 0);
        this->foward_relate_ipv6_query(t_pkt, t_rr);
        break;
    case LDNS_RR_TYPE_AAAA:
        qDebug()<<"query IPv6, ok";
        break;
    default:
        qDebug()<<"unimpled rr type:"<<t;
        break;
    }

    this->foward_resolve_query(data, len);

    // cleanup 
    ldns_pkt_free(t_pkt);
    if (qit) delete qit;
}

void KDNS::processResponse(char *data, int len, QHostAddress host, quint16 port)
{
    qDebug()<<len<<host<<port;

    ldns_pkt *t_pkt = NULL, *t_pkt2 = NULL;
    ldns_rr_list *t_rr_list = NULL;
    ldns_rr *t_rr = NULL, *t_rr2 = NULL;
    ldns_rdf *t_rdf = NULL;
    ldns_status t_status;
    char t_buff[256];
    char *t_ptr = NULL;
    size_t t_len;
    QueueItem *qit = new QueueItem();
    QueueItem *qit2 = NULL;
    bool rewrite_response = false;
    
    t_status = ldns_wire2pkt(&t_pkt, (const uint8_t*)data, len);
    t_rr_list = ldns_pkt_answer(t_pkt);
    qit->m_qid = ldns_pkt_id(t_pkt);

    qit2 = this->find_item_by_qid(qit->m_qid);
    if (qit2) qit2->debug();
    else {
        qDebug()<<"can not find queued item for:"<<qit->m_qid<<m_relate_ipv6_query.contains(qit->m_qid);
        qDebug()<<m_qrqueue.keys();
    }

    qDebug()<<"rrlistcount:"<<ldns_rr_list_rr_count(t_rr_list);
    for (int i = ldns_rr_list_rr_count(t_rr_list) - 1; i >= 0 ; i --) {
        t_rr = ldns_rr_list_rr(t_rr_list, i);
        
        qDebug()<<i<<t_rr;
        switch (ldns_rr_type t = ldns_rr_get_type(t_rr)) {
        case LDNS_RR_TYPE_A:
            qDebug()<<i<<"got IPv4 ipaddr:";
            if (qit2) {
                qit2->m_got4 = true;
            }

            if (this->need_hijacking_domain(t_rr)) {
                // a emu wrong ip, DNS劫持
                t_rdf = ldns_rr_rdf(t_rr, 0);
                qDebug()<<"rdf size:"<<ldns_rdf_size(t_rdf);
                inet_pton(AF_INET, "127.0.0.1", t_buff); // 可能本机开了服务的情况，防止发出错误请求
                memcpy(ldns_rdf_data(t_rdf), t_buff, 4);
                rewrite_response = true;
            }

            // 已经处理过DNS劫持了，后面的IPv4地址可以忽略掉。但需要保证留下一个劫持IPv4地址
            if (rewrite_response && i > 0) {
                t_rr2 = ldns_rr_list_rr(t_rr_list, i - 1);
                if (ldns_rr_get_type(t_rr2) == LDNS_RR_TYPE_A) {
                    t_rr2 = ldns_rr_list_pop_rr(t_rr_list);
                    if (t_rr2) {
                        ldns_rr_free(t_rr2); // cleanup
                        t_rr2 = NULL;
                    } else {
                        qDebug()<<"null pop rr detected.";
                    }
                }
            }
            
            break;
        case LDNS_RR_TYPE_AAAA:
            qDebug()<<"got IPv6 ipaddr:";
            this->fill_domain_has_ipv6(t_rr);
            if (qit2) {
                qit2->m_got6 = true;
            }
            break;
        default:
            qDebug()<<"unimpled rr type:"<<t;
            break;
        }
    }

    qDebug()<<"forward back to:"<<qit2<<rewrite_response<<m_qrqueue.size();
    if (qit2) {
        if (rewrite_response) {
            ldns_pkt2wire((uint8_t**)&t_ptr, t_pkt, &t_len);
            m_sock->writeDatagram(t_ptr, t_len, qit2->m_addr, qit2->m_port);
        } else {
            m_sock->writeDatagram(data, len, qit2->m_addr, qit2->m_port);
        }
    }

    // cleanup
    if (t_ptr) free(t_ptr);
    ldns_pkt_free(t_pkt);
    delete qit; qit = NULL;
    if (qit2) {
        m_qrqueue.remove(qit2->hash());
        delete qit2;
    }
}

bool KDNS::foward_resolve_query(char *data, int len)
{
    m_fwd_sock->writeDatagram(data, len, QHostAddress(UPS_DNS_SERVER_ADDR), UPS_DNS_SERVER_PORT);
    return true;
}

bool KDNS::foward_relate_ipv6_query(ldns_pkt *pkt, ldns_rr *rr)
{
    // need IPv6 query
    ldns_pkt *t_pkt = pkt;
    ldns_rr *t_rr = rr;
    char *t_ptr = NULL;
    size_t t_len = 0;

    ldns_rr_set_type(t_rr, LDNS_RR_TYPE_AAAA);
    ldns_pkt_set_random_id(t_pkt);
    m_relate_ipv6_query[ldns_pkt_id(t_pkt)] = true;

    ldns_pkt2wire((uint8_t**)&t_ptr, t_pkt, &t_len);
    this->foward_resolve_query(t_ptr, t_len);
    m_fwd_sock->waitForBytesWritten(); // hope that ipv6 first response

    // cleanup
    free(t_ptr);
    
    return true;
}

QueueItem *KDNS::find_item_by_qid(quint16 qid)
{
    QueueItem *item = NULL;
    QHash<QString, QueueItem*>::iterator it;
    
    for (it = this->m_qrqueue.begin(); it != this->m_qrqueue.end(); it ++) {
        item = *it;

        if (item->m_qid == qid) {
            break;
        }
        
        item = NULL;
    }

    return item;
}

/**
   所有拥有IPv6地址的域名都需要劫持
 */
bool KDNS::need_hijacking_domain(ldns_rr *rr)
{
    QString dname;
    ldns_rdf *t_rdf = NULL;
    t_rdf = ldns_rr_owner(rr);

    switch (ldns_rdf_type t = ldns_rdf_get_type(t_rdf)) {
    case LDNS_RDF_TYPE_DNAME:
        dname = QString(ldns_rdf2str(t_rdf));
        break;
    default:
        break;
    }

    if (dname.isEmpty()) return false;

    if (m_domain_has_ipv6.contains(dname) && m_domain_has_ipv6[dname] == true) {
        return true;
    }

    return false;
}

// unused now
bool KDNS::do_hijacking_domain(ldns_pkt *pkt, ldns_rr *rr)
{
    ldns_pkt *t_pkt = pkt;
    ldns_rr *t_rr = rr;
    ldns_rdf *t_rdf = NULL;
    char *t_ptr = NULL;
    size_t t_len = 0;
    char *t_buff[256];

    // a emu wrong ip, DNS劫持
    t_rdf = ldns_rr_rdf(t_rr, 0);
    qDebug()<<"rdf size:"<<ldns_rdf_size(t_rdf);
    inet_pton(AF_INET, "127.0.0.", t_buff); // 可能本机开了服务的情况，防止发出错误请求
    memcpy(ldns_rdf_data(t_rdf), t_buff, 4);
    ldns_pkt2wire((uint8_t**)&t_ptr, t_pkt, &t_len);
    
    return true;
}

// 把域名解析出来填入到hasipv6中
// called if ipv6
bool KDNS::fill_domain_has_ipv6(ldns_rr *rr)
{
    QString dname;
    ldns_rdf *t_rdf = NULL;
    t_rdf = ldns_rr_owner(rr);

    switch (ldns_rdf_type t = ldns_rdf_get_type(t_rdf)) {
    case LDNS_RDF_TYPE_DNAME:
        dname = QString(ldns_rdf2str(t_rdf));
        if (m_domain_has_ipv6.contains(dname)) {
        } else {
            m_domain_has_ipv6[dname] = true;
        }
        break;
    default:
        break;
    }

    return true;
}

void KDNS::debugPacket(char *data, int len)
{
    ldns_pkt *t_pkt = NULL, *t_pkt2 = NULL;
    ldns_rr_list *t_rr_list = NULL;
    ldns_rr *t_rr = NULL;
    ldns_rdf *t_rdf = NULL;
    ldns_status t_status;
    char t_buff[256];
    char *t_ptr = NULL;
    size_t t_len;

    t_status = ldns_wire2pkt(&t_pkt, (const uint8_t*)data, len);

    qDebug()<<"ps:"<<ldns_pkt_size(t_pkt)
            <<"qid:"<<ldns_pkt_id(t_pkt)
            <<"qr:"<<ldns_pkt_qr(t_pkt)
            <<"rd:"<<ldns_pkt_rd(t_pkt)
            <<"ra:"<<ldns_pkt_ra(t_pkt)
            <<"rcode:"<<ldns_pkt_get_rcode(t_pkt)
            <<"qd:"<<ldns_pkt_qdcount(t_pkt)
            <<"an:"<<ldns_pkt_ancount(t_pkt)
            <<"question ptr:"<<ldns_pkt_question(t_pkt)
            <<"answer ptr:"<<ldns_pkt_answer(t_pkt);

    if (ldns_pkt_qr(t_pkt) == 0) {
        t_rr_list = ldns_pkt_question(t_pkt);
    } else {
        t_rr_list = ldns_pkt_answer(t_pkt);
    }

    for (int i = 0; i < ldns_rr_list_rr_count(t_rr_list); i ++) {
        t_rr = ldns_rr_list_rr(t_rr_list, i);

        qDebug()<<"    rrcount:"<<ldns_rr_list_rr_count(t_rr_list)<<i;
        qDebug()<<"    rdfcount:"<<ldns_rr_rd_count(t_rr);
        qDebug()<<"    rr1type:"<<ldns_rr_get_type(t_rr);

        t_rdf = ldns_rr_owner(t_rr);
        qDebug()<<"    owner type:"<<ldns_rdf_get_type(t_rdf)
                <<"    owner size:"<<ldns_rdf_size(t_rdf);
        switch (ldns_rdf_type t = ldns_rdf_get_type(t_rdf)) {
        case LDNS_RDF_TYPE_DNAME:
            qDebug()<<"own doamin:"<<(char*)ldns_rdf2str(t_rdf);
            break;
        default:
            qDebug()<<"unknown owner:"<<t;
            break;
        }

        for (int j = 0; j < ldns_rr_rd_count(t_rr); j ++) {
            t_rdf = ldns_rr_rdf(t_rr, j);
            qDebug()<<"        rdf type:"<<ldns_rdf_get_type(t_rdf);
            switch (ldns_rdf_type t = ldns_rdf_get_type(t_rdf)) {
            case LDNS_RDF_TYPE_A:
                inet_ntop(AF_INET, ldns_rdf_data(t_rdf), t_buff, sizeof(t_buff));
                qDebug()<<"ip:"<<t_buff;
                break;
            case LDNS_RDF_TYPE_AAAA:
                inet_ntop(AF_INET6, ldns_rdf_data(t_rdf), t_buff, sizeof(t_buff));
                qDebug()<<"ip:"<<t_buff;
                break;
            default:
                qDebug()<<"unknown rdf type:"<<t;
                break;
            }
        }
    }

    fprintf(stdout, "\n");

    // cleanup 
    ldns_pkt_free(t_pkt);
}

/*
  dnsmasq 可以使用另一个hosts文件，可以把dnsmasq指向本dns server,
  
  如果某个域名有IPv6的IP则把这个域名记录到hosts文件中，

  然后系统的dns指向dnsmasq，这样就能实现通过hosts返回ipv6地址了。

  另一种简单的方式，使用DNS劫持，如果主机有IPv6地址，则把IPv4地址劫持到127.0.0.1，
  一般程序都会重试另一个ip，而这个ip则是真实的IPv6的IP了。

  TODO: 内存泄漏
 */
