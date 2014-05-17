#ifndef _SRUDP_H_
#define _SRUDP_H_

#include <QtCore>
#include <QtNetwork>

class StunClient;

class SruPacket
{
public:
    uint8_t command;
    uint8_t opt;
    uint16_t reliable_ack;
    uint16_t reliable;
    uint16_t unreliable;

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
    Srudp(StunClient *stun);
    virtual ~Srudp();

    bool sendto(QByteArray data, QString host, quint16 port);
    bool sendto(QByteArray data, QString host);
    bool hasPendingDatagrams();
    QByteArray readDatagram(QHostAddress &addr, quint16 &port);

public slots:
    void onRawPacketRecieved(QByteArray pkt, QString peer_addr);
    void onPacketRecieved(QJsonObject jobj);
    // lost packet retrive
    void onRetranLostPacketFinished(QNetworkReply *reply);

signals:
    void readyRead();
    void newPacket(QByteArray pkt);
    void realClientDisconnected();
    void resetSenderState();

private:
    // bool verifyPacket(QJsonObject jobj);
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
    QMap<QString, int> m_traned_seqs; // "seq" => 1/0
    static qint64 m_pkt_seq; 

    /////////// 
public:
    // rfc rudp proto
    enum { CMD_NOOP = 0, CMD_CLOSE = 1, CMD_CONN_REQ, CMD_CONN_RSP, CMD_PING, CMD_PONG,
           CMD_APP = 0x10, CMD_APP_MAX = 0xff - 0x10};
    enum { OPT_NONE = 0, OPT_RELIABLE = 1, OPT_ACK = 2, OPT_RETRANSMITED = 4};

    // bool connectToHost(QString host, quint16 port);
    // bool disconnectFromHost();
    bool serverConnectToHost(QString host, quint16 port);
    bool setHost(QString host, quint16 port);
    bool setClientMode(bool is_client);
    
public slots:
    bool ping();
    bool rawProtoPacketHandler(QJsonObject jobj);
    // bool protoConnectHandler(QJsonObject jobj);
    // bool protoConnectedHandler(QJsonObject jobj);
    bool protoPingHandler(QJsonObject jobj);
    bool protoPongHandler(QJsonObject jobj);
    // bool protoCloseHandler(QJsonObject jobj);

signals:
    void connected();
    void connectError();

private slots:
    void onSendConfirmTimeout();

private:
    bool schedPrepareSendto(QString peer_addr, QJsonObject jobj);
    bool schedRealSendto(QJsonObject jobj);
    bool schedNextSendto();

private:
    // rfc rudp proto
    bool m_client_mode = false; // call connectToHost is client, and it's stun peerA
    QString m_proto_host;
    quint16 m_proto_port = 0;
    bool m_proto_must_ack = 1;
    QVector<QJsonObject> m_proto_sched_queue; // 发送调度队列，大小为10，应该能影响更及时，减小包拥塞
    static const int m_proto_sched_queue_max_size = 10; // 
    QVector<QJsonObject> m_proto_send_queue;
    QTimer *m_proto_send_confirm_timer = NULL;
    static const int m_proto_confirm_timeout = 1000 * 1; // ms
    static const int m_proto_resend_max_times = 16;
    static const int m_proto_ping_max_timeout = 1000 * 6;
    QDateTime m_proto_last_ping_time = QDateTime::currentDateTime();
    QUdpSocket *m_proto_sock = NULL;
};

/*
  https://github.com/fbx/librudp
 */
#endif /* _SRUDP_H_ */
