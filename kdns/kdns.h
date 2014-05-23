#ifndef _KDNS_H_
#define _KDNS_H_

#include <QtCore>
#include <QtNetwork>

#include "queue.h"

class KDNS : public QObject
{
    Q_OBJECT;
public:
    explicit KDNS();
    virtual ~KDNS();

    void init();
               
    void processQuery(char *data, int len, QHostAddress host, quint16 port);
    void processResponse(char *data, int len, QHostAddress host, quint16 port);

public slots:
    void onReadyRead();
    void onFwdReadyRead();

private:
    QueueItem *find_item_by_qid(quint16 qid);
    void debugPacket(char *data, int len);

private:
    QUdpSocket *m_sock = NULL;
    static const quint16 m_port = 53;
    QUdpSocket *m_fwd_sock = NULL;

    QHash<QString, QueueItem*> m_qrqueue;
};


#endif
