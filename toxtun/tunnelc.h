#ifndef _TUNNELC_H_
#define _TUNNELC_H_

#include <QtCore>
#include <QtNetwork>

#include "enet/enet.h"

// class ToxNet;
// class Srudp;
class QToxKit;

class Tunnelc : public QObject
{
    Q_OBJECT;
public:
    Tunnelc();
    virtual ~Tunnelc();

    void init();

public slots:
    void onTunnelConnected();
    void onTunnelDisconnected();
    void onTunnelReadyRead();

    ///////
    void onToxnetSelfConnectionStatus(int status);
    void onToxnetFriendConnectionStatus(QString pubkey, int status);
    void onToxnetFriendMessage(QString pubkey, int type, QByteArray message);

    
private slots:
    void onNewTcpConnection();
    void onTcpReadyRead();
    void onTcpDisconnected();

private:
public:
    // ToxNet *m_net = NULL;
    // Srudp *m_rudp = NULL;

    QTcpServer *m_tcpsrv = NULL;

    QToxKit *m_toxkit = NULL;
    ENetHost *m_encli = NULL;
    
    QHash<QString, QVector<QByteArray> > m_pkts;  // friendId => [pkt1/2/3]
};



#endif /* _TUNNELC_H_ */
