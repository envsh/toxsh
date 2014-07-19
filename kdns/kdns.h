#ifndef _KDNS_H_
#define _KDNS_H_

#include <QtCore>
#include <QtNetwork>

#include <ldns/ldns.h>

#include "queue.h"

class KDNS : public QObject
{
    Q_OBJECT;
public:
    explicit KDNS();
    virtual ~KDNS();

    void init();
               
    void processQuery(char *data, int len, QHostAddress host, quint16 port, QTcpSocket *sock);
    void processResponse(char *data, int len, QHostAddress host, quint16 port);

public slots:
    void onReadyRead();
    void onFwdReadyRead();

    void onTcpServerNewConnection();
    void onTcpServerDisconnected();

    void onTcpServerReadyRead();

private:
    bool foward_resolve_query(char *data, int len);
    bool foward_relate_ipv6_query(ldns_pkt *pkt, ldns_rr *rr);
    QueueItem *find_item_by_qid(quint16 qid);
    bool need_hijacking_domain(ldns_rr *rr);
    bool do_hijacking_domain(ldns_pkt *pkt, ldns_rr *rr);
    bool fill_domain_has_ipv6(ldns_rr *rr);
    void debugPacket(char *data, int len);

private:
    QUdpSocket *m_sock = NULL;
    QTcpServer *m_tcp_sock = NULL;
    static const quint16 m_port = 5353;
    QUdpSocket *m_fwd_sock = NULL;

    QHash<QString, QueueItem*> m_qrqueue; // item-hash => item
    QHash<quint16, bool> m_relate_ipv6_query;
    QHash<QString, bool> m_domain_has_ipv6;  // domain => hashipv6
    QHash<QTcpSocket*, QueueItem*> m_jcqueue; // just connected, no data. when receive data, move to m_qrqueue
};


#endif
