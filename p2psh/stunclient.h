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
    bool sendIndication(QByteArray data);
    bool channelBind(QString peer_addr);
    bool channelData(QByteArray data);
    bool refresh();
    bool sendRelayData(QByteArray data, QString relayed_addr);

public slots:
    void onStunConnected();
    void onStunReadyRead();
    void onRetryTimeout();

signals:
    void mappedAddressRecieved(QString addr);
    void packetRecieved(QByteArray ba);
    void allocateDone();
    void channelBindDone(QString relayed_addr);

private:
    void processResponse(QByteArray resp);
    static void debugStunResponse(QByteArray resp);
    static QString getStunAddress(QByteArray resp, uint16_t attr_type);
    static void printHexView(unsigned char *buf, size_t len);

private:
    QUdpSocket *m_stun_sock = NULL;
    QTimer *m_retry_timer = NULL;
    quint16 m_stun_port = 0;

    QByteArray m_realm;
    QByteArray m_nonce;
    QByteArray m_integrity;
    QString m_relayed_addr;
    QString m_mapped_addr;
    uint32_t m_lifetime;
    
    QString m_peer_addr;

    // reliable
    QTimer *m_sending_timer = NULL;
    bool m_sending_udp = false;
    QByteArray m_sending_data;
    QString m_sending_addr;
    
};

#endif /* _STUNCLIENT_H_ */
