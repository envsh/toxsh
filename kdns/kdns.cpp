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

QHostAddress g_addr;
quint16 g_port;
QByteArray g_first_pkt;

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
        }
        g_addr = addr;
        g_port = port;
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
            ptr = data;
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
        }
    }
    qDebug()<<"read pkt count:"<<cnter;
}


