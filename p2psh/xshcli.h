#ifndef _XSHCLI_H_
#define _XSHCLI_H_

#include <QtCore>
#include <QtNetwork>

#include "peersrv.h"


class XshCli : public PeerSrv
{
    Q_OBJECT;
public:
    XshCli();
    virtual ~XshCli();

    virtual void init();

public slots:
    virtual void onRelayReadyRead(); 
    void onAllocateDone();
    void onChannelBindDone(QString relayed_addr);
    
    void onNewBackendConnection();
    void onBackendReadyRead();

protected:
    virtual QString getRegCmd();

private:
    QTcpServer *m_backend_sock = NULL;
    QTcpSocket *m_conn_sock = NULL;
    QString m_peer_addr;
    QString m_peer_relayed_addr;
    bool  m_channel_done = false;
};

#endif /* _XSHCLI_H_ */
