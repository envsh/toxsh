
// #include "toxnet.h"
// #include "srudp.h"

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "enet/enet.h"
#include "bilayer.h"

#include "qtoxkit.h"
#include "serializer.h"

#include "enetpoll.h"
#include "toxtunutils.h"

#include "tunneld.h"


////////////////////////
Tunneld::Tunneld(QString config_file)
    : ToxTunBase()
{
    m_cfg = new ToxTunConfig(config_file);
}

Tunneld::~Tunneld()
{

}

static int toxenet_socket_send (ENetSocket socket, const ENetAddress * address,
                                const ENetBuffer * buffers, size_t bufferCount,
                                ENetPeer *enpeer, void *user_data)
{
    Tunneld *tund = (Tunneld*)user_data;
    QToxKit *toxkit = tund->m_toxkit;    
    // ToxTunChannel *chan = tund->m_enpeer_chans[enpeer];
    // ToxTunChannel *chan = tund->m_conid_chans[(uint32_t)(enpeer->data)];
    // ToxTunChannel *chan = (ToxTunChannel*)enpeer->toxchan;
    ToxTunChannel *chan = tund->peerLastChan(enpeer);
    // qDebug()<<bufferCount<<toxkit<<enpeer<<chan;

    if (enpeer == NULL) {}
    if (chan == NULL) {
        qDebug()<<"maybe connect/ping/disconnect."<<enpeer
                <<"i/o:"<<enpeer->incomingPeerID<<enpeer->outgoingPeerID;
    }
    
    size_t sentLength = 0;
    // QString friendId = "451843FCB684FC779B302C08EA33DDB447B61402B075C8AF3E84BEE5FB41C738928615BCDCE4";
    // QString friendId = QString(address->toxid);
    QString friendId = toxkit->friendGetPublicKey(address->vaddr);
    int idsrc = 0;
    if (chan != NULL) { // connected state
        // qDebug()<<chan;
        // qDebug()<<chan->m_peer_pubkey;
        friendId = chan->m_peer_pubkey;
        idsrc = 1;
    } else { // not connected state
        // friendId = QString(address->toxid);
        friendId = toxkit->friendGetPublicKey(address->vaddr);
        idsrc = 2;
    }
    // qDebug()<<"sending to:"<<friendId<<QString(enpeer->toxid)<<idsrc;
    Q_ASSERT(friendId.length() > 0);
    if (friendId.length() == 0) {
        // send to who???
        // return 0;
    }
    
    QByteArray data = serialize_packet(address, buffers, bufferCount);
    bool bret = tund->m_toxkit->friendSendLossyPacket(friendId, data);
    if (!bret) { return 0; }
    
    // tund->m_toxkit->friendSendMessage(friendId, data);
    for (int i = 0; i < bufferCount; i ++) {
        sentLength += buffers[i].dataLength;  
    }

    // for (int i = 0; i < bufferCount; i++) {
    //     QByteArray data = serialize_packet(address, &buffers[i]);
    //     toxkit->friendSendMessage(QString(), data);
    // }

    return sentLength;
    return 0;
}


static int toxenet_socket_receive(ENetSocket socket, ENetAddress *address,
                                  ENetBuffer *buffers, size_t bufferCount,
                                  ENetEvent *event, void *user_data)
{
    Tunneld *tund = (Tunneld*)user_data;
    QToxKit *toxkit = tund->m_toxkit;    
    // ToxTunChannel *chan = tund->m_enpeer_chans[event->peer];
    // qDebug()<<event;
    // qDebug()<<event->peer;
    // qDebug()<<event->peer->connectID;

    ToxTunChannel *chan = NULL;
    // ToxTunChannel *chan = tund->m_conid_chans[event->peer->connectID];
    // qDebug()<<bufferCount<<toxkit<<event->peer<<chan;

    if (event->peer == NULL) {}
    if (chan == NULL) {}
    QMutexLocker mtlck(&tund->m_pkts_mutex);
    if (tund->m_inpkts.count() == 0) {
        return 0;
    }

    if (bufferCount != 1) {
        qDebug()<<"not supported >1:" << bufferCount;
    }
    
    struct sockaddr_in sin = {0};
    QString pubkey = tund->m_inpkts.begin().key();

    if (tund->m_inpkts[pubkey].count() > 0) {
        // qDebug()<<tund<<socket<<bufferCount;
        
        int recvLength = 0;
        // QByteArray pkt = tund->m_pkts[pubkey].takeAt(0);
        // QByteArray pkt = tund->m_pkts[pubkey].at(0);
        // tund->m_pkts[pubkey].remove(0);
        QByteArray pkt = tund->m_inpkts[pubkey].dequeue();
        
        // deserialize
        recvLength = deserialize_packet(pkt, address, &buffers[0]);
        if (address != NULL) {
            address->vaddr = toxkit->friendByPublicKey(pubkey);
            /*
            if (strlen(address->toxid) == 0 || strcmp(address->toxid, pubkey.toLatin1().data()) != 0) {
                qDebug()<<"need reset unsame toxid:"
                        <<address->toxid<<"->"<<pubkey;
                strcpy(address->toxid, pubkey.toLatin1().data());
            } else {
                // strcpy(address->toxid, pubkey.toLatin1().data());
            }
            */
        }
        // qDebug()<<tund<<socket<<bufferCount<<recvLength;
        return recvLength;
    }
    
    return 0;
}


void Tunneld::init()
{
    
    // m_net = new ToxNet;
//     m_rudp = new Srudp(m_net);

//     QObject::connect(m_net, &ToxNet::peerConnected, this, &Tunneld::onPeerConnected);
//     QObject::connect(m_net, &ToxNet::peerDisconnected, this, &Tunneld::onPeerDisconnected);
//     QObject::connect(m_rudp, &Srudp::readyRead, this, &Tunneld::onPeerReadyRead);

    QString tkname = this->m_cfg->m_srvname;
    qDebug()<<tkname;
    // m_toxkit = new QToxKit("whtun", true);
    m_toxkit = new QToxKit(tkname, true);
    QObject::connect(m_toxkit, &QToxKit::selfConnectionStatus, this,
                     &Tunneld::onToxnetSelfConnectionStatus, Qt::QueuedConnection);
    QObject::connect(m_toxkit, &QToxKit::friendConnectionStatus, this,
                     &Tunneld::onToxnetFriendConnectionStatus, Qt::QueuedConnection);
    QObject::connect(m_toxkit, &QToxKit::friendRequest, this,
                     &Tunneld::onToxnetFriendRequest, Qt::QueuedConnection);
    QObject::connect(m_toxkit, &QToxKit::friendMessage, this,
                     &Tunneld::onToxnetFriendMessage, Qt::QueuedConnection);
    QObject::connect(m_toxkit, &QToxKit::friendLossyPacket, this,
                     &Tunneld::onToxnetFriendLossyPacket, Qt::QueuedConnection);
    QObject::connect(m_toxkit, &QToxKit::friendLosslessPacket, this,
                     &Tunneld::onToxnetFriendLosslessPacket, Qt::QueuedConnection);
    m_toxkit->start();

    m_enpoll = new ENetPoll();
    QObject::connect(m_enpoll, &ENetPoll::connected, this, &Tunneld::onENetPeerConnected, Qt::QueuedConnection);
    QObject::connect(m_enpoll, &ENetPoll::disconnected, this, &Tunneld::onENetPeerDisconnected, Qt::QueuedConnection);
    QObject::connect(m_enpoll, &ENetPoll::packetReceived, this, &Tunneld::onENetPeerPacketReceived, Qt::QueuedConnection);
    m_enpoll->start();

    
    ENetAddress enaddr = {ENET_HOST_ANY, 7767};
    enet_address_set_host(&enaddr, "127.0.0.1");
    enaddr.vaddr = SELF_FRIEND_NUMBER;
    ENetHost *ensrv = NULL;

    // m_ensrv = ensrv = enet_host_create(&enaddr, 932, 2, 0, 0);
    m_ensrv = ensrv = enet_host_create_notp(&enaddr, 932, 2, 0, 0);
    // ensrv->mtu = 1271; // 在这设置无效  // ENET_HOST_DEFAULT_MTU=1400
    // m_ensrv = ensrv = enet_host_create(&enaddr, 0, 2, 0, 0);
    qDebug()<<ensrv<<ensrv->peerCount;
    ensrv->toxkit = this;
    ensrv->enet_socket_send = toxenet_socket_send;
    ensrv->enet_socket_receive = toxenet_socket_receive;
    // ensrv->compressor.context = (void*)0x1;
    // ensrv->compressor.compress = enet_simple_compress;
    // ensrv->compressor.decompress = enet_simple_decompress;
    // ensrv->compressor.destroy = enet_simple_destroy;
    // enet_host_compress(ensrv, this->createCompressor());


    // transport
    const ENetTransport *transport = this->createToxTransport();
    // m_transport.send = toxenet_socket_send;
    // m_transport.recv = toxenet_socket_receive;
    m_transport.enet_transport_send = toxenet_socket_send;
    m_transport.enet_transport_recv = toxenet_socket_receive;
    enet_host_transport(ensrv, transport);

    
    m_enpoll->addENetHost(ensrv);

    QObject::connect(this, &Tunneld::testRunThread, m_enpoll, &ENetPoll::testRunThread);
    emit(this->testRunThread());
}

static int g_conn_stat = 0;
void Tunneld::onPeerConnected(int friendId)
{
    qDebug()<<friendId;

    if (g_conn_stat == 0) {
        g_conn_stat = 1;
    } else {
        return;
    }

    m_sock = new QTcpSocket();

    QObject::connect(m_sock, &QTcpSocket::readyRead, this, &Tunneld::onTcpReadyRead, Qt::QueuedConnection);
    QObject::connect(m_sock, &QTcpSocket::disconnected, this, &Tunneld::onTcpDisconnected, Qt::QueuedConnection);

    m_sock->connectToHost("127.0.0.1", 22);
    m_sock->waitForConnected();
}

void Tunneld::onPeerDisconnected(int friendId)
{
    qDebug()<<friendId;
}

void Tunneld::onPeerReadyRead()
{
    qDebug()<<"";

    QHostAddress addr;
    quint16 port;

    // while (m_rudp->hasPendingDatagrams()) {
    //     QByteArray ba = m_rudp->readDatagram(addr, port);
    //     qDebug()<<"read balen:"<<ba.length();

    //     m_sock->write(ba);
    // }
}

void Tunneld::onToxnetSelfConnectionStatus(int status)
{
    qDebug()<<status;
    
}

void Tunneld::onToxnetFriendConnectionStatus(QString pubkey, int status)
{
    qDebug()<<pubkey<<status;
}

void Tunneld::onToxnetFriendRequest(QString pubkey, QString reqmsg)
{
    qDebug()<<pubkey<<reqmsg;
    m_toxkit->friendAddNorequest(pubkey);   
}

void Tunneld::onToxnetFriendMessage(QString pubkey, int type, QByteArray message)
{
    // qDebug()<<pubkey<<type<<message;
    if (type == TOX_MESSAGE_TYPE_ACTION) {
        return;
    }
    QMutexLocker mtlck(&m_pkts_mutex);
    // put buffers
    if (!m_inpkts.contains(pubkey)) {
        m_inpkts[pubkey] = QQueue<QByteArray>();
    }

    m_inpkts[pubkey].append(message);
}

//////////////////
void Tunneld::onENetPeerConnected(ENetHost *enhost, ENetPeer *enpeer, quint32 data)
{
    qDebug()<<enhost<<enpeer<<enpeer->address.vaddr<<data<<".";

    QTcpSocket *sock = new QTcpSocket();
    QObject::connect(sock, &QTcpSocket::readyRead, this, &Tunneld::onTcpReadyRead, Qt::QueuedConnection);
    QObject::connect(sock, &QTcpSocket::disconnected, this, &Tunneld::onTcpDisconnected, Qt::QueuedConnection);

    enpeer->timeoutLimit *= 10;
    enpeer->timeoutMinimum *= 10;
    enpeer->timeoutMaximum *= 10;

    ToxTunChannel *chan = new ToxTunChannel();
    chan->m_sock = sock;
    chan->m_enpeer = enpeer;
    chan->m_enhost = enhost;
    chan->m_peer_pubkey = "";
    // chan->m_peer_pubkey = QString(enpeer->toxid);
    chan->m_peer_pubkey = m_toxkit->friendGetPublicKey(enpeer->address.vaddr);
    chan->m_host = "127.0.0.1";
    chan->m_port = data;  // connect to port
    chan->m_conid = this->nextConid();


    if (peerChansCount(enpeer) > 0) {
        int cc0 = peerChansCount(enpeer);
        qDebug()<<enpeer->incomingPeerID<<cc0;
        ToxTunChannel *tchan = peerLastChan(enpeer);
        qDebug()<<tchan<<tchan->m_conid
                <<tchan->sock_closed<<tchan->peer_sock_closed<<tchan->enet_closed;
        // promiseChannelCleanup(tchan);

        // 无条件把相关联的chan清理掉
        if (tchan->sock_closed == true && tchan->peer_sock_closed == false) {
            qDebug()<<"need force cleanup:"<<tchan->m_conid;
            tchan->peer_sock_closed = true;
            tchan->force_closed = true;
            promiseChannelCleanup(tchan); 
        }
        else
        if (tchan->sock_closed == false && tchan->peer_sock_closed == false) {
            qDebug()<<"need force cleanup:"<<tchan->m_conid;
            tchan->peer_sock_closed = true;
            tchan->sock_closed = true;
            tchan->force_closed = true;
            promiseChannelCleanup(tchan); 
        }

        int cc1 = peerChansCount(enpeer);
        if (cc1 != 0) {
            qDebug()<<enpeer->incomingPeerID<<cc0;            
        }
        assert(peerChansCount(enpeer) == 0);
        // assert(1 == 2);
    }
    peerAddChan(enpeer, chan);


    this->m_sock_chans[sock] = chan;
    // this->m_enpeer_chans[enpeer] = chan;
    this->m_conid_chans[chan->m_conid] = chan;

    // sock->connectToHost("127.0.0.1", 8118);
    sock->connectToHost(chan->m_host, chan->m_port);
    sock->waitForConnected();
}

void Tunneld::onENetPeerDisconnected(ENetHost *enhost, ENetPeer *enpeer)
{
    qDebug()<<enhost<<enpeer;
    // ToxTunChannel *chan = this->m_enpeer_chans[enpeer];
    // ToxTunChannel *chan = this->m_conid_chans[uint32_t(enpeer->data)];
    // ToxTunChannel *chan = (ToxTunChannel*)enpeer->toxchan;
    ToxTunChannel *chan = peerLastChan(enpeer);

    if (chan != NULL) {
        qDebug()<<"warning why chan not null:"<<chan->m_conid;
    } else {
        assert(chan == NULL);
    }
    // chan->enet_closed = true;
    // this->promiseChannelCleanup(chan);
}

void Tunneld::onENetPeerPacketReceived(ENetHost *enhost, ENetPeer *enpeer, int chanid, QByteArray packet)
{
    // ToxTunChannel *chan = this->m_enpeer_chans[enpeer];
    // ToxTunChannel *chan = this->m_conid_chans[(enpeer->data)];
    // ToxTunChannel *chan = (ToxTunChannel*)enpeer->toxchan;
    ToxTunChannel *chan = peerLastChan(enpeer);

    if (packet.length() > 50) {
        // qDebug()<<enhost<<enpeer<<chanid<<packet.length()<<packet.left(20)<<"..."<<packet.right(20);
    } else {
        // qDebug()<<enhost<<enpeer<<chanid<<packet.length()<<packet;
    }

    if (chan == NULL) {
        qDebug()<<"error: chan null:"<<enpeer;
        if (packet.length() > 50) {
            qDebug()<<enhost<<enpeer<<chanid<<packet.length()<<packet.left(20)<<"..."<<packet.right(20);
        } else {
            qDebug()<<enhost<<enpeer<<chanid<<packet.length()<<packet;
        }
        qDebug()<<"drop crash packet, be careful, be true";
        return;
        // 这个断言在外网还是出现比较频繁的。
        assert(1 == 2);
    }
    
    if (chanid == 1) {
        qDebug()<<enhost<<enpeer<<chanid<<packet.length()<<packet;
        if (packet == QByteArray("CLIFIN")) {
            if (chan->peer_sock_closed == false) {
                chan->peer_sock_closed = true;
                chan->peer_sock_close_time = QDateTime::currentDateTime();

                if (chan->sock_closed == false) {
                    // should close myself socket???
                    // 也就是对方不会再发送新的数据过来，也不会再接收数据了，
                    // 但有可能对方的发送缓冲区中还有未发送成功的缓冲数据。
                    // 使用计时器，如果在一定时间内再也没有收到包，则认为对方没有要发送的数据了。
                }
                
                this->promiseChannelCleanup(chan);                
            } else {
                qDebug()<<"maybe duplicate CLIFIN packet:i/o:"
                        <<enpeer->incomingPeerID<<enpeer->outgoingPeerID;
            }
        } else {
            qDebug()<<"invalid chan1 packet:";
        }
        return;
    }

    chan->last_recv_peer_pkt_time = QDateTime::currentDateTime();
    QTcpSocket *sock = chan->m_sock;

    int wrlen = sock->write(packet);
}

/////////////////
void Tunneld::onTcpConnected()
{
    QTcpSocket *sock = (QTcpSocket*)sender();    
}


void Tunneld::onTcpDisconnected()
{
    qDebug()<<""<<enet_host_used_peer_size(m_ensrv)<<m_sock_chans.count()<<m_conid_chans.count();
    QTcpSocket *sock = (QTcpSocket*)(sender());
    ToxTunChannel *chan = this->m_sock_chans[sock];
    // qDebug()<<chan<<chan->m_enpeer->state;

    chan->sock_closed = true;
    chan->sock_close_time = QDateTime::currentDateTime();
    
    // 防止对方没有收完，暂时还不能关闭连接。
    // enet_peer_disconnect_later(chan->m_enpeer, qrand());

    if (true) {
        QByteArray ba = "SRVFIN";
        ENetPacket *packet = enet_packet_create(ba.data(), ba.length(), ENET_PACKET_FLAG_RELIABLE);

        // enet_packet_resize(packet, 13);
        // strcpy((char*)&packet->data[9], "foo");
    
        uint8_t chanid = 1;
        ENetPeer *enpeer = chan->m_enpeer;
        // enet_peer_send(enpeer, chanid, packet);
        m_enpoll->sendPacket(enpeer, chanid, packet);
    }

    this->promiseChannelCleanup(chan);
}

void Tunneld::onTcpReadyRead()
{
    qDebug()<<"";
    QTcpSocket *sock = (QTcpSocket*)sender();
    ToxTunChannel *chan = this->m_sock_chans[sock];

    qDebug()<<chan<<chan->m_enpeer->state;

    if (chan->m_enpeer->state == ENET_PEER_STATE_CONNECTED) {
        while (sock->bytesAvailable() > 0) {
            // QByteArray ba = sock->readAll();
            QByteArray ba = sock->read(567);
            if (ba.length() >= 1371) {
                qDebug()<<"too long data packet.";
            }
        
            ENetPacket *packet = enet_packet_create(ba.data(), ba.length(), ENET_PACKET_FLAG_RELIABLE);

            // enet_packet_resize(packet, 13);
            // strcpy((char*)&packet->data[9], "foo");
    
            uint8_t chanid = 0;
            ENetPeer *enpeer = chan->m_enpeer;
            // enet_peer_send(enpeer, chanid, packet);
            m_enpoll->sendPacket(enpeer, chanid, packet);
        }
    }

    
    // QByteArray ba = m_sock->readAll();
    // m_rudp->sendto(ba, QString("127.0.0.1:6789"));
}


void Tunneld::promiseChannelCleanup(ToxTunChannel *chan)
{
    qDebug()<<chan<<sender();
    QObject *snderobj = (QObject*)sender();
    QTimer *repeat_timer = NULL;

    qDebug()<<snderobj->objectName()<<snderobj->metaObject()->className();
    if (chan == NULL) {
        repeat_timer = (QTimer*)snderobj;
        assert(repeat_timer != NULL);
        int conid = repeat_timer->property("conid").toInt();
        if (!m_conid_chans.contains(conid)) {
            qDebug()<<"maybe too late repeat check self sock close timer event";
            repeat_timer->deleteLater();
            return;
        }
        chan = m_conid_chans[conid];
        assert(chan != NULL);
    } else {
        // snderobj is ENetPoll or QTcpSocket
    }
    QTcpSocket *sock = chan->m_sock;
    ENetPeer *enpeer = chan->m_enpeer;

    //////////
    QHash<QString, bool> promise_results;

    promise_results["sock_closed"] = chan->sock_closed;
    // promise_results["enet_closed"] = chan->enet_closed;
    promise_results["peer_sock_closed"] = chan->peer_sock_closed;

    bool promise_result = true;
    for (auto it = promise_results.begin(); it != promise_results.end(); it ++) {
        QString key = it.key();
        bool val = it.value();
        promise_result = promise_result && val;
    }

    if (true) {
        // 检测对方最近的回包情况
        if (!promise_result && repeat_timer == NULL
            && promise_results["peer_sock_closed"] && !promise_results["sock_closed"]) {
            qDebug()<<"here";
            if (chan->last_recv_peer_pkt_time == QDateTime()) {
                qDebug()<<"maybe can close socket right now, because recv nothing forever";
            }
            
            QTimer *t = new QTimer();
            t->setInterval(500);
            t->setSingleShot(true);
            t->setProperty("conid", QVariant(chan->m_conid));
            // // QObject::connect(t, &QTimer::timeout, this, &Tunneld::promiseChannelCleanup, Qt::QueuedConnection);
            QObject::connect(t, SIGNAL(timeout()), this, SLOT(promiseChannelCleanup()), Qt::QueuedConnection);
            qDebug()<<"start repeat check sock close timer:";
            t->start();
        }
        if (!promise_result && repeat_timer != NULL
            && promise_results["peer_sock_closed"] && !promise_results["sock_closed"]) {
            //
            QDateTime now_time = QDateTime::currentDateTime();
            uint32_t last_recv_to_now_time = chan->last_recv_peer_pkt_time.msecsTo(now_time);
            qDebug()<<"here:"<<last_recv_to_now_time<<enpeer->lastReceiveTime;
            if (last_recv_to_now_time > 7000) {
                qDebug()<<"last recv to now, force close self socket:"<<last_recv_to_now_time
                        <<enpeer->incomingPeerID<<enpeer->outgoingPeerID;
                // 不能直接关闭，要在当前函数执行完后，即下一次事件的时候开始执行。
                QTimer::singleShot(1, sock, &QTcpSocket::close);
                // QTimer *t = new QTimer();
                // t->setSingleShot(true);
                // QObject::connect(t, &QTimer::timeout, sock, &QTcpSocket::close, Qt::QueuedConnection);
                // t->start(1);
                
                repeat_timer->deleteLater();
            } else {
                repeat_timer->start();
            }
        }
    }
    
    if (!promise_result) {
        qDebug()<<"promise nooooot satisfied:"<<promise_results<<chan->m_conid;
        return;
    }
    
    chan->promise_close_time = QDateTime::currentDateTime();
    qDebug()<<"promise satisfied."<<chan->m_conid;

    ///// do cleanup
    bool force_closed = chan->force_closed;
    
    // enpeer->toxchan = NULL;  // cleanup
    peerRemoveChan(enpeer, chan);
    
    this->m_sock_chans.remove(sock);
    // this->m_enpeer_chans.remove(enpeer);
    this->m_conid_chans.remove(chan->m_conid);

    delete chan;
    sock->disconnect();
    sock->deleteLater();
    if (repeat_timer != NULL) repeat_timer->deleteLater();
    qDebug()<<"curr chan size:"<<this->m_sock_chans.count()<<this->m_conid_chans.count();

    if (force_closed) {
        return;
    }

    // 延时关闭enet_peer
    QTimer *t = new QTimer();
    auto later_close_timeout = [enpeer, t]() {
        qDebug()<<enpeer<<enpeer->state;
        if (enpeer->state != ENET_PEER_STATE_CONNECTED) {
            qDebug()<<"warning, peer currently not connected:"<<enpeer->incomingPeerID;
        }

        if (! (enet_list_empty (& enpeer -> outgoingReliableCommands) &&
               enet_list_empty (& enpeer -> outgoingUnreliableCommands) && 
               enet_list_empty (& enpeer -> sentReliableCommands))) {
            qDebug()<<"warning, maybe has unsent packet:"<<enpeer->incomingPeerID;
        }

        qDebug()<<"last recv time:"<<enpeer->incomingPeerID
        <<enetx_time_diff(enpeer->lastReceiveTime, enet_time_get());

        qDebug()<<"restore peer timeout, ping interval";
        enet_peer_timeout(enpeer, ENET_PEER_TIMEOUT_LIMIT*2,
                          ENET_PEER_TIMEOUT_MINIMUM*2, ENET_PEER_TIMEOUT_MAXIMUM*2);
        enet_peer_ping_interval(enpeer, ENET_PEER_PING_INTERVAL*2);

        // enet_peer_disconnect_now(enpeer, qrand());
        enet_peer_disconnect_later(enpeer, qrand());
        t->deleteLater();
    };

    qDebug()<<"last recv time:"<<enpeer->incomingPeerID
            <<enetx_time_diff(enpeer->lastReceiveTime, enet_time_get());
    // QTimer::singleShot(5678, later_close_timeout);
    t->setInterval(5678);
    t->setSingleShot(true);
    QObject::connect(t, &QTimer::timeout, later_close_timeout);
    t->start();
}

//////////////////
