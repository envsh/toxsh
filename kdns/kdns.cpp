#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <arpa/nameser_compat.h>

#include "ares_dns.h"
#include "ares.h"
#include "kdns.h"

#include <ldns/ldns.h>

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
    char data[2560];
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

            /*
            ptr = data;
            ldns_wire2pkt(&qpkt, (const uint8_t*)ptr, iret);
            rr_list = ldns_pkt_question(qpkt);

            qDebug()<<iret<<"qid:"<<DNS_HEADER_QID(ptr)
                    <<"qr:"<<DNS_HEADER_QR(ptr)
                    <<"aa:"<<DNS_HEADER_AA(ptr)
                    <<"rd:"<<DNS_HEADER_RD(ptr)
                    <<"ra:"<<DNS_HEADER_RA(ptr)
                    <<"qd:"<<DNS_HEADER_QDCOUNT(ptr)
                    <<"an:"<<DNS_HEADER_ANCOUNT(ptr)
                    <<"ns:"<<DNS_HEADER_NSCOUNT(ptr)
                    <<"ar:"<<DNS_HEADER_ARCOUNT(ptr)
                    <<"qt:"<<DNS_QUESTION_TYPE(ptr+31)
                    <<"qc:"<<DNS_QUESTION_CLASS(ptr+31);
            for (int i = 0; i < ldns_rr_list_rr_count(rr_list); i ++) {
                rr = ldns_rr_list_rr(rr_list, i);
                qDebug()<<i
                        <<"ldns qt:"<<ldns_rr_get_type(rr)
                        <<"ldns qc:"<<ldns_rr_get_class(rr);
                if (ldns_rr_get_type(rr) == LDNS_RR_TYPE_A) {
                    ldns_rr_set_type(rr, LDNS_RR_TYPE_AAAA);
                }
            }

            qDebug()<<QByteArray(data, iret).toHex();
            if (g_first_pkt.isEmpty()) g_first_pkt = QByteArray(data+2, iret-2);
            else {
                qDebug()<<(QByteArray(data, iret) == g_first_pkt);
                // qDebug()<<g_first_pkt.toHex();
            }
            if (QByteArray(data+2, iret-2) != g_first_pkt) {
                
            }
            
            // modify question
            // DNS_QUESTION_SET_TYPE(ptr+31, 28);
            ptr = NULL;
            ldns_pkt2wire((uint8_t**)&ptr, qpkt, &ilen);
            qDebug()<<"pkt 2 buff:"<<ilen;

            m_fwd_sock->writeDatagram(ptr, ilen, QHostAddress("2001:4860:4860::8888"), 53);
            // m_fwd_sock->writeDatagram(data, iret, QHostAddress("2001:4860:4860::8888"), 53);
            */
        }
    }
    qDebug()<<"read pkt count:"<<cnter;
}

void KDNS::onFwdReadyRead()
{
    qDebug()<<"";
    
    QHostAddress addr;
    quint16 port;
    char data[2560];
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
            /*
            ptr = data;
            this->processResponse(data, iret, addr, port);
            qDebug()<<iret<<"qid:"<<DNS_HEADER_QID(ptr)
                    <<"rd:"<<DNS_HEADER_RD(ptr)
                    <<"ra:"<<DNS_HEADER_RA(ptr)
                    <<"qd:"<<DNS_HEADER_QDCOUNT(ptr)
                    <<"an:"<<DNS_HEADER_ANCOUNT(ptr);
            
            irc = ares_parse_a_reply((unsigned char*)ptr, iret, &host, NULL, &naddrttls);
            if (host != NULL) {
                qDebug()<<irc<<host<<naddrttls<<host->h_name<<host->h_aliases<<host->h_addr_list
                        <<host->h_addrtype<<AF_INET<<AF_INET6;
                // if AF_INET, this is A rec, transform to AAAA rec

                p = host->h_aliases[0];
                for (int i =0 ; p != NULL; p = host->h_aliases[i++]) {
                    qDebug()<<"alias:"<<p;
                }

                p = host->h_addr_list[0];
                for (int i = 0 ; p != NULL; p = host->h_addr_list[++i]) {
                    char straddr[64];
                    char straddr6[64];
                    inet_ntop(AF_INET, p, straddr, sizeof(straddr));
                    qDebug()<<"addr:"<<(void*)p<<straddr<<QByteArray(p, 4).toHex();
                }
            }

            host = NULL;
            irc = ares_parse_aaaa_reply((unsigned char*)ptr, iret, &host, NULL, &naddrttls);
            if (host != NULL) {
                qDebug()<<irc<<host<<naddrttls<<host->h_name<<host->h_addrtype<<AF_INET<<AF_INET6;

                p = host->h_addr_list[0];
                for (int i = 0 ; p != NULL; p = host->h_addr_list[++i]) {
                    char straddr[64];
                    inet_ntop(AF_INET6, p, straddr, sizeof(straddr));
                    qDebug()<<"addr:"<<(void*)p<<straddr;
                }
                
            } else {
                qDebug()<<"can not got ipv6 addr";
            }
                        
            m_sock->writeDatagram(data, iret, g_addr, g_port);
            */
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
    }

    t_rr_list = ldns_pkt_question(t_pkt);
    switch (ldns_rr_type t = ldns_rr_list_type(t_rr_list)) {
    case LDNS_RR_TYPE_A:
        // need IPv6 query
        t_rr = ldns_rr_list_rr(t_rr_list, 0);
        ldns_rr_set_type(t_rr, LDNS_RR_TYPE_AAAA);
        ldns_pkt2wire((uint8_t**)&t_ptr, t_pkt, &t_len);
        // m_fwd_sock->writeDatagram(t_ptr, t_len, QHostAddress("2001:4860:4860::8888"), 53);
        // m_fwd_sock->waitForBytesWritten();
        m_fwd_sock->writeDatagram(data, len, QHostAddress("2001:4860:4860::8888"), 53);
        
        
        t_pkt2 = ldns_pkt_new();
        ldns_pkt_set_id(t_pkt2, qit->m_qid);
        ldns_pkt_set_qr(t_pkt2, LDNS_PACKET_IQUERY);
        // ldns_pkt_set_rcode(t_pkt2, LDNS_RCODE_NOTIMPL);
        ldns_pkt_set_rcode(t_pkt2, LDNS_RCODE_NXDOMAIN);
        
        t_ptr = NULL;
        ldns_pkt2wire((uint8_t**)&t_ptr, t_pkt2, &t_len);
        // m_sock->writeDatagram(t_ptr, t_len, host, port);
        break;
    case LDNS_RR_TYPE_AAAA:
        qDebug()<<"query IPv6, ok";
        m_fwd_sock->writeDatagram(data, len, QHostAddress("2001:4860:4860::8888"), 53);
        break;
    default:
        qDebug()<<"unimpled rr type:"<<t;
        m_fwd_sock->writeDatagram(data, len, QHostAddress("2001:4860:4860::8888"), 53);
        break;
    }
}

void KDNS::processResponse(char *data, int len, QHostAddress host, quint16 port)
{
    ldns_pkt *t_pkt = NULL, *t_pkt2 = NULL;
    ldns_rr_list *t_rr_list = NULL;
    ldns_rr *t_rr = NULL;
    ldns_status t_status;
    char t_buff[256];
    char *t_ptr = NULL;
    size_t t_len;
    QueueItem *qit = new QueueItem();
    QueueItem *qit2 = NULL;
    
    t_status = ldns_wire2pkt(&t_pkt, (const uint8_t*)data, len);
    t_rr_list = ldns_pkt_answer(t_pkt);
    qit->m_qid = ldns_pkt_id(t_pkt);

    qit2 = this->find_item_by_qid(qit->m_qid);
    if (qit2) qit2->debug();
    else {
        qDebug()<<"can not find queued item for:"<<qit->m_qid;
    }

    for (int i = 0; i < ldns_rr_list_rr_count(t_rr_list); i ++) {
        t_rr = ldns_rr_list_rr(t_rr_list, i);
        switch (ldns_rr_type t = ldns_rr_get_type(t_rr)) {
        case LDNS_RR_TYPE_A:
            qDebug()<<"got IPv4 ipaddr:";
            if (qit2) {
                qit2->m_got4 = true;
            }
            break;
        case LDNS_RR_TYPE_AAAA:
            qDebug()<<"got IPv6 ipaddr:";
            if (qit2) {
                qit2->m_got6 = true;
            }
            break;
        default:
            qDebug()<<"unimpled rr type:"<<t;
            break;
        }
    }

    if (qit2) {
        m_sock->writeDatagram(data, len, qit2->m_addr, qit2->m_port);
    }
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
            <<"rd:"<<ldns_pkt_rd(t_pkt)
            <<"ra:"<<ldns_pkt_ra(t_pkt)
            <<"qd:"<<ldns_pkt_qdcount(t_pkt)
            <<"an:"<<ldns_pkt_ancount(t_pkt)
            <<"question ptr:"<<ldns_pkt_question(t_pkt)
            <<"answer ptr:"<<ldns_pkt_answer(t_pkt);

    t_rr_list = ldns_pkt_answer(t_pkt);
    for (int i = 0; i < ldns_rr_list_rr_count(t_rr_list); i ++) {
        t_rr = ldns_rr_list_rr(t_rr_list, i);

        qDebug()<<"    rrcount:"<<ldns_rr_list_rr_count(t_rr_list)<<i;
        qDebug()<<"    rdfcount:"<<ldns_rr_rd_count(t_rr);
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
}
