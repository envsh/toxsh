#ifndef _DHTPROC_H_
#define _DHTPROC_H_

#include <QtCore>

#include <tox/DHT.h>

class DhtProc : public QObject
{
    Q_OBJECT;
public:
    DhtProc();
    virtual ~DhtProc();

    void start();
    void stop();

signals:
    void pubkeyDone(QByteArray pubkey);
    void connected(int conn);
    void dhtSizeChanged(int size);

private slots:
    void process();

private:
    QTimer *m_timer = NULL;
    DHT *m_dht = NULL;
    Networking_Core *m_net_core = NULL;
    int m_dht_size = 0;
    int m_connected = 0;
};

#endif /* _DHTPROC_H_ */
