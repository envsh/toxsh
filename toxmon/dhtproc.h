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
    void dhtNodesChanged(int friendCount, int clientCount, int ping_array_size, int harden_ping_array_size);

    void closeNodes(const QStringList &nodes);

private slots:
    void process();
    void analysis();
    void recordNodes();

private:
    QTimer *m_timer = NULL;
    DHT *m_dht = NULL;
    Networking_Core *m_net_core = NULL;
    int m_dht_size = 0;
    int m_connected = 0;

    QHash<QString, int> m_pinged; // ping过来的所有节点记录
};

#endif /* _DHTPROC_H_ */
