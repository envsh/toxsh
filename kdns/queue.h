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
    quint16 m_port;
    QByteArray m_query;
    QDateTime m_time;
    int m_qid;
    bool m_got4;
    bool m_got6;
};

#endif /* _QUEUE_H_ */
