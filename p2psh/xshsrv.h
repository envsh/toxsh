#ifndef _XSHSRV_H_
#define _XSHSRV_H_


#include <QtCore>
#include <QtNetwork>

class StunClient;
class Srudp;

class XshSrv : public QObject
{
    Q_OBJECT;
public:
    XshSrv();
    virtual ~XshSrv();

    void init();

public slots:
    void onMappedAddressRecieved(QString addr);
    void onAllocateDone(QString relayed_addr);
    void onChannelBindDone(QString relayed_addr);
    void onPacketRecieved(QByteArray pkt);
    void onPacketReadyRead();

    void onRelayConnected();
    void onRelayReadyRead();
    void onBackendConnected();
    void onBackendDisconnected();
    void onBackendReadyRead();

    void onRudpConnected();
    void onRudpConnectError();

private:
    StunClient *m_stun_client = NULL;
    QTcpSocket *m_rly_sock = NULL;
    QTcpSocket *m_backend_sock = NULL;
    QString m_mapped_addr;
    QString m_peer_addr;
    QString m_peer_relayed_addr;
    Srudp *m_rudp = NULL;
};

#endif /* _XSHSRV_H_ */
