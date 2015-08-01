#ifndef _TUNNELD_H_
#define _TUNNELD_H_

#include <QtCore>
#include <QtNetwork>

#include "enet/enet.h"

// class ToxNet;
// class Srudp;
class QToxKit;

class Tunneld : public QObject
{
    Q_OBJECT;
public:
    Tunneld();
    virtual ~Tunneld();

    void init();

public slots:
    void onPeerConnected(int friendId);
    void onPeerDisconnected(int friendId);
    void onPeerReadyRead();

    ///////
    void onToxnetSelfConnectionStatus(int status);
    void onToxnetFriendConnectionStatus(QString pubkey, int status);
    void onToxnetFriendRequest(QString pubkey, QString reqmsg);
    void onToxnetFriendMessage(QString pubkey, int type, QByteArray message);
                                                                           
private slots:
    void onTcpConnected();
    void onTcpDisconnected();
    void onTcpReadyRead();

private:
    // ToxNet *m_net = NULL;
    // Srudp *m_rudp = NULL;

    QTcpSocket *m_sock = NULL;
    QToxKit *m_toxkit = NULL;
    ENetHost *m_ensrv = NULL;
};

#endif /* _TUNNELD_H_ */
