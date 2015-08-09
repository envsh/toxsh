#ifndef _TUNNELC_H_
#define _TUNNELC_H_

#include <QtCore>
#include <QtNetwork>

#include "enet/enet.h"

#include "toxtunbase.h"

// class ToxNet;
// class Srudp;
class QToxKit;
class ToxTunChannel;
class ToxTunConfig;
class ENetPoll;



class Tunnelc : public ToxTunBase
{
    Q_OBJECT;
public:
    Tunnelc(QString config_file = "./toxtun_whttp.ini");
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
    void onENetPeerConnected(ENetHost *enhost, ENetPeer *enpeer, quint32 data);
    void onENetPeerDisconnected(ENetHost *enhost, ENetPeer *enpeer);
    void onENetPeerPacketReceived(ENetHost *enhost, ENetPeer *enpeer, int chanid, QByteArray packet);
    
    void onNewTcpConnection();
    void onTcpReadyRead();
    void onTcpDisconnected();

public slots:
    void promiseChannelCleanup(ToxTunChannel *chan = NULL);
    
private:
public:
    // ToxNet *m_net = NULL;
    // Srudp *m_rudp = NULL;
    ToxTunConfig *m_cfg = NULL;

    QHash<QTcpServer*, int> m_tcpsrvs;
    // QTcpServer *m_tcpsrv = NULL;

    QToxKit *m_toxkit = NULL;
    ENetHost *m_encli = NULL;

    ENetPoll *m_enpoll = NULL;
    
    // QHash<QString, QVector<QByteArray> > m_pkts;  // friendId => [pkt1/2/3] // move to base
    // QHash<void*, ToxTunChannel*> m_chans;  // sock=>chan, enhost=>chan, enpeer=>chan
    QHash<QTcpSocket*, ToxTunChannel*> m_sock_chans;
    // QHash<ENetPeer*, ToxTunChannel*> m_enpeer_chans;
    QHash<uint32_t, ToxTunChannel*> m_conid_chans;
};



#endif /* _TUNNELC_H_ */
