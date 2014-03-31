#ifndef _VIRTTCPD_H_
#define _VIRTTCPD_H_

#include <QtCore>
#include <QtNetwork>

class VirtTcpD : public QObject
{
    Q_OBJECT;
public:
    VirtTcpD();
    virtual ~VirtTcpD();
    void init();

public slots:
    void onNewConnection();
    void onClientReadyRead();
    // void onClientConnected();
    void onClientDisconnected();
    void onClientError(QAbstractSocket::SocketError socketError);
    // void onClientHostFound();
    // void onClientStateChanged(QAbstractSocket::SocketState socketState);
    void onPacketRecieved(QJsonObject jobj);

signals:
    void newPacket(QByteArray pkt);
    void realClientDisconnected();

private:
    bool verifyPacket(QJsonObject jobj);
    bool cachePacket(QJsonObject jobj);
    QJsonObject getNextPacket();
    
private:
    QTcpServer *m_tcpd = NULL;
    QMap<QTcpSocket*, QTcpSocket*> m_conns;
    qint64 m_last_pkt_seq = 0;
    QMap<qint64, QJsonObject> m_cached_pkt_seqs; // 缓存非正常顺序到达的包编号，等待后续包到了之后再发送出去
};

#endif /* _VIRTTCPD_H_ */
