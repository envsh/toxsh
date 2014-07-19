#ifndef _STUNCLIENT_H_
#define _STUNCLIENT_H_

#include <QtCore>
#include <QtNetwork>

class StunClient : public QObject
{
    Q_OBJECT;
public:
    StunClient(quint16 port);
    virtual ~StunClient();

    bool getMappedAddress();
    bool allocate(char *realm = NULL, char *nonce = NULL);
    bool createPermission(QString peer_addr);
    bool sendIndication(QString peer_addr, QByteArray data);
    bool replyIndication(QString peer_addr, QByteArray data);
    bool channelBind(QString peer_addr);
    bool channelData(QByteArray data);
    bool refresh();
    bool sendRelayData(QByteArray data, QString relayed_addr);

public slots:
    void onStunConnected();
    void onStunReadyRead();
    void onRetryTimeout();
    void onRefreshTimeout();
    void onPermKATimeout();

signals:
    void mappedAddressRecieved(QString addr);
    void packetReceived(QByteArray ba, QString peer_addr);
    void allocateDone(QString relayed_addr);
    void createPermissionDone();
    void stunError();
    void channelBindDone(QString relayed_addr);

private:
    void processResponse(QByteArray resp, QString peer_addr);
    static void debugStunResponse(QByteArray resp);
    static QString getStunAddress(QByteArray resp, uint16_t attr_type);
    static void printHexView(unsigned char *buf, size_t len);
    static QString getMethodName(int method);
    void saveAllocatePuples(QByteArray realm, QByteArray nonce);
    void loadAllocatePuples();

private:
    QUdpSocket *m_stun_sock = NULL;
    quint16 m_stun_port = 0;

    QByteArray m_realm;
    QByteArray m_nonce;
    QByteArray m_integrity;
    QString m_relayed_addr;
    QString m_mapped_addr;
    uint32_t m_lifetime;
    static const int m_channel_refresh_timeout = 1000 * 60 * 1; // todo change to m_allocate_refresh_timeout
    QTimer *m_channel_refresh_timer = NULL;
    static const int m_permission_keepalive_timeout = 1000 * 6 * 3; // 注，这里不用*5是因为可能由于别的请求导致keepalive延时，不能精确到5
    QTimer *m_permission_keepalive_timer = NULL;
   
    QString m_peer_addr;

    // reliable
    QTimer *m_sending_timer = NULL;
    bool m_sending_udp = false;
    QByteArray m_sending_data;
    QString m_sending_addr;
    
};

#endif /* _STUNCLIENT_H_ */
