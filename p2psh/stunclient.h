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

public slots:
    void onStunConnected();
    void onStunReadyRead();
    void onRetryTimeout();

signals:
    void mappedAddressRecieved(QString addr);
    void packetRecieved(QByteArray ba);

private:
    void processResponse(QByteArray resp);

private:
    QUdpSocket *m_stun_sock = NULL;
    QTimer *m_retry_timer = NULL;
    quint16 m_stun_port = 0;
};

#endif /* _STUNCLIENT_H_ */
