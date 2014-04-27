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

    // lost packet retrive
    void onRetranLostPacketFinished(QNetworkReply *reply);

signals:
    void newPacket(QByteArray pkt);
    void realClientDisconnected();
    void resetSenderState();

private:
    bool verifyPacket(QJsonObject jobj);
    bool cachePacket(QJsonObject jobj);
    QJsonObject getNextPacket();
    void guessLostPacket();
    void retranLostPacket(qint64 lost_seq, QString from_id, QString to_id);
    
private:
    QTcpServer *m_tcpd = NULL;
    QMap<QTcpSocket*, QTcpSocket*> m_conns;
    qint64 m_last_pkt_seq = 0;
    QString m_last_pkt_id;
    QMap<qint64, QJsonObject> m_cached_pkt_seqs; // 缓存非正常顺序到达的包编号，等待后续包到了之后再发送出去
    QMap<QString, int> m_retran_urls;
    QMap<QString, int> m_traned_seqs;
    QNetworkAccessManager *m_nam = NULL;
};

#endif /* _VIRTTCPD_H_ */
