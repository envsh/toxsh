#ifndef _SRUDP_H_
#define _SRUDP_H_

#include <QtCore>
#include <QtNetwork>

class StunClient;

class SruPacket
{
public:
    qint64 m_pkt_id;
    qint64 m_prev_seq;
    qint64 m_curr_seq;
    QDateTime ctime;
    QDateTime mtime;
    QByteArray data; // hex mode
    int dlen;
    QString addr;
    quint16 port;
    int retry;
};

class Srudp : public QObject
{
    Q_OBJECT;
public:
    enum { CMD_NOOP = 0, CMD_CLOSE = 1, CMD_CONN_REQ, CMD_CONN_RSP, CMD_PING, CMD_PONG, CMD_APP = 0x10};

    Srudp(StunClient *stun);
    virtual ~Srudp();

    bool sendto(QByteArray data, QString host, quint16 port);
    bool sendto(QByteArray data, QString host);
    bool hasPendingDatagrams();
    QByteArray readDatagram(QHostAddress &addr, quint16 &port);

public slots:
    void onRawPacketRecieved(QByteArray pkt);
    void onPacketRecieved(QJsonObject jobj);
    // lost packet retrive
    void onRetranLostPacketFinished(QNetworkReply *reply);

signals:
    void readyRead();
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
    StunClient *m_stun_client = NULL;
    qint64 m_out_last_pkt_seq = 0;
    qint64 m_in_last_pkt_seq = 0;
    QString m_last_pkt_id;
    QMap<qint64, QJsonObject> m_cached_pkt_seqs; // m_in_caches;
    QMap<qint64, QJsonObject> m_out_caches;
    QMap<QString, int> m_traned_seqs; // "seq" => 1/0
    static qint64 m_pkt_seq; 
};

/*
  https://github.com/fbx/librudp
 */
#endif /* _SRUDP_H_ */
