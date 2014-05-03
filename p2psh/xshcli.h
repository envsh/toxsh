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
    
    void onNewBackendConnection();
    void onBackendReadyRead();

protected:
    virtual QString getRegCmd();

private:
    QTcpServer *m_backend_sock = NULL;
};

#endif /* _XSHCLI_H_ */
