#ifndef _QUEUE_H_
#define _QUEUE_H_

#include <QtCore>
#include <QtNetwork>

class QueueItem : public QObject
{
    Q_OBJECT;
public:
    explicit QueueItem();
    virtual ~QueueItem();

    QString hash();
    
    void debug();

public:
    QHostAddress m_addr;
    quint16 m_port = 0;
    QByteArray m_query;
    QDateTime m_time;
    int m_qid = -1;
    bool m_got4 = false;
    bool m_got6 = false;
    bool m_use_tcp = false;
    QTcpSocket *m_tcp_cli = NULL;
};

#endif /* _QUEUE_H_ */
