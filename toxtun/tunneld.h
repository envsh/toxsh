#ifndef _TUNNELD_H_
#define _TUNNELD_H_

#include <QtCore>
#include <QtNetwork>

#include "enet/enet.h"

#include "toxtunbase.h"


class QToxKit;
class ToxTunChannel;
class ToxTunConfig;
class ENetPoll;


class Tunneld : public ToxTunBase
{
    Q_OBJECT;
public:
    Tunneld(QString config_file = "./toxtun_whttp.ini");
    virtual ~Tunneld();

    void init();

public slots:
    ///////
    void onToxnetSelfConnectionStatus(int status);
    void onToxnetFriendConnectionStatus(QString pubkey, int status);
    void onToxnetFriendRequest(QString pubkey, QString reqmsg);
    void onToxnetFriendMessage(QString pubkey, int type, QByteArray message);

private slots:
    void onENetPeerConnected(ENetHost *enhost, ENetPeer *enpeer, quint32 data);
    void onENetPeerDisconnected(ENetHost *enhost, ENetPeer *enpeer);
    void onENetPeerPacketReceived(ENetHost *enhost, ENetPeer *enpeer, int chanid, QByteArray packet);
    
    void onTcpConnected();
    void onTcpDisconnected();
    void onTcpReadyRead();

public slots:
    void promiseChannelCleanup(ToxTunChannel *chan = NULL);

signals:
    void testRunThread();
    
private:
public:
    ToxTunConfig *m_cfg = NULL;

    QTcpSocket *m_sock = NULL;
    // QToxKit *m_toxkit = NULL;
    ENetHost *m_ensrv = NULL;
    ENetPoll *m_enpoll = NULL;
    
    // QHash<QString, QVector<QByteArray> > m_pkts;  // friendId => [pkt1/2/3]
    // QHash<void*, ToxTunChannel*> m_chans;  // sock=>chan, enhost=>chan, enpeer=>chan
    QHash<QTcpSocket*, ToxTunChannel*> m_sock_chans;
    // QHash<ENetPeer*, ToxTunChannel*> m_enpeer_chans;
    QHash<uint32_t, ToxTunChannel*> m_conid_chans;
    
};

#endif /* _TUNNELD_H_ */
