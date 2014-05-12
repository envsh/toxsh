#ifndef _XSHCLI_H_
#define _XSHCLI_H_

#include <QtCore>
#include <QtNetwork>

#include "peersrv.h"

class Srudp;

class XshCli : public PeerSrv
{
    Q_OBJECT;
public:
    XshCli();
    virtual ~XshCli();

    virtual void init();

public slots:
    virtual void onRelayReadyRead(); 
    void onAllocateDone(QString relayed_addr);
    void onCreatePermissionDone();
    // void onChannelBindDone(QString relayed_addr);
    void onPacketRecieved(QByteArray pkt);
    void onPacketReadyRead();
    
    void onNewBackendConnection();
    void onBackendReadyRead();
    void onBackendDisconnected();

    void onRudpConnected();
    void onRudpConnectError();

protected:
    virtual QString getRegCmd();

private:
    QTcpServer *m_backend_sock = NULL;
    QTcpSocket *m_conn_sock = NULL;
    QString m_relayed_addr = NULL;
    QString m_peer_addr;
    // QString m_peer_relayed_addr;
    // bool  m_channel_done = false;
    bool m_perm_done = false;
    Srudp *m_rudp = NULL;
};

#endif /* _XSHCLI_H_ */
