// #include "toxnet.h"
// #include "srudp.h"

#include "enet/enet.h"

#include "qtoxkit.h"
#include "serializer.h"
#include "toxtunutils.h"
#include "enetpoll.h"

#include "tunnelc.h"


////////////////////////

Tunnelc::Tunnelc(QString config_file)
    : ToxTunBase()
{
    m_cfg = new ToxTunConfig(config_file);
}

Tunnelc::~Tunnelc()
{

}


static int toxenet_socket_send (ENetSocket socket, const ENetAddress * address,
                                const ENetBuffer * buffers, size_t bufferCount,
                                ENetPeer *enpeer, void *user_data)
{
    Tunnelc *tunc = (Tunnelc*)user_data;    
    QToxKit *toxkit = tunc->m_toxkit;

    // ToxTunChannel *chan = tunc->m_enpeer_chans[enpeer];
    // ToxTunChannel *chan = (ToxTunChannel*)enpeer->toxchan;
    ToxTunChannel *chan = tunc->peerLastChan(enpeer);
    // qDebug()<<bufferCount<<toxkit<<enpeer<<chan;

    if (enpeer == NULL) {}
    if (chan == NULL) {
        qDebug()<<"maybe connect/ping/disconnect."<<enpeer
                <<"i/o:"<<enpeer->incomingPeerID<<enpeer->outgoingPeerID;
    }

    size_t sentLength = 0;
    QString friendId = QString(address->toxid);
    int idsrc = 0;
    if (chan != NULL) { // connected state
        friendId = chan->m_peer_pubkey;
        idsrc = 1;
    } else { // not connected state
        friendId = QString(address->toxid);
        idsrc = 2;
    }
    // qDebug()<<"sending to:"<<friendId<<QString(enpeer->toxid)<<idsrc;
    if (friendId.length() == 0) {
        // why?????
    }
    
    QByteArray data = serialize_packet(address, buffers, bufferCount);
    bool bret = toxkit->friendSendLossyPacket(friendId, data);
    if (!bret) { return 0; }
    
    // toxkit->friendSendMessage(friendId, data);
    for (int i = 0; i < bufferCount; i ++) {
        sentLength += buffers[i].dataLength;
    }

    // for (int i = 0; i < bufferCount; i++) {
    //     QByteArray data = serialize_packet(address, &buffers[i]);
    //     toxkit->friendSendMessage(friendId, data);
    //     sentLength += buffers[i].dataLength;
    // }

    return sentLength;
    return 0;
}


static int toxenet_socket_receive(ENetSocket socket, ENetAddress *address,
                                  ENetBuffer *buffers, size_t bufferCount,
                                  ENetEvent *event, void *user_data)
{
    Tunnelc *tunc = (Tunnelc*)user_data;
    QToxKit *toxkit = tunc->m_toxkit;

    ToxTunChannel *chan = NULL;
    // ToxTunChannel *chan = tunc->m_enpeer_chans[event->peer];
    // ToxTunChannel *chan = tunc->m_conid_chans[event->peer->connectID];
    // qDebug()<<bufferCount<<toxkit<<event->peer<<chan;

    if (event->peer == NULL) {}
    if (chan == NULL) {}

    QMutexLocker mtlck(&tunc->m_pkts_mutex);
    if (tunc->m_inpkts.count() == 0) {
        return 0;
    }

    if (bufferCount != 1) {
        qDebug()<<"not supported >1:" << bufferCount;
    }
    
    struct sockaddr_in sin = {0};
    QString pubkey = tunc->m_inpkts.begin().key();

    if (tunc->m_inpkts[pubkey].count() > 0) {
        // qDebug()<<tunc<<socket<<bufferCount;
        
        int recvLength = 0;
        // QByteArray pkt = tunc->m_pkts[pubkey].takeAt(0);
        // QByteArray pkt = tunc->m_pkts[pubkey].at(0);
        // tunc->m_pkts[pubkey].remove(0);
        QByteArray pkt = tunc->m_inpkts[pubkey].dequeue();
        
        // deserialize
        recvLength = deserialize_packet(pkt, address, &buffers[0]);
        
        // qDebug()<<tunc<<socket<<bufferCount<<recvLength;
        return recvLength;
    }
    
    return 0;
}

void Tunnelc::init()
{
    // m_net = new ToxNet;
    // m_rudp = new Srudp(m_net);
    // m_rudp->setClientMode(true);

    // QObject::connect(m_net, &ToxNet::netConnected, this, &Tunnelc::onTunnelConnected);
    // QObject::connect(m_net, &ToxNet::netDisconnected, this, &Tunnelc::onTunnelDisconnected);
    // QObject::connect(m_rudp, &Srudp::readyRead, this, &Tunnelc::onTunnelReadyRead);

    m_toxkit = new QToxKit("toxcli", true);
    QObject::connect(m_toxkit, &QToxKit::selfConnectionStatus, this,
                     &Tunnelc::onToxnetSelfConnectionStatus, Qt::QueuedConnection);
    QObject::connect(m_toxkit, &QToxKit::friendConnectionStatus, this,
                     &Tunnelc::onToxnetFriendConnectionStatus, Qt::QueuedConnection);
    QObject::connect(m_toxkit, &QToxKit::friendMessage, this,
                     &Tunnelc::onToxnetFriendMessage, Qt::QueuedConnection);
    QObject::connect(m_toxkit, &QToxKit::friendLossyPacket, this,
                     &Tunnelc::onToxnetFriendLossyPacket, Qt::QueuedConnection);
    QObject::connect(m_toxkit, &QToxKit::friendLosslessPacket, this,
                     &Tunnelc::onToxnetFriendLosslessPacket, Qt::QueuedConnection);
    m_toxkit->start();

    m_enpoll = new ENetPoll();
    QObject::connect(m_enpoll, &ENetPoll::connected, this, &Tunnelc::onENetPeerConnected, Qt::QueuedConnection);
    QObject::connect(m_enpoll, &ENetPoll::disconnected, this, &Tunnelc::onENetPeerDisconnected, Qt::QueuedConnection);
    QObject::connect(m_enpoll, &ENetPoll::packetReceived, this, &Tunnelc::onENetPeerPacketReceived, Qt::QueuedConnection);
    m_enpoll->start();

    ENetAddress enaddr = {ENET_HOST_ANY, 7766};
    enet_address_set_host(&enaddr, "127.0.0.1");
    ENetHost *encli = NULL;

    m_encli = encli = enet_host_create(&enaddr, 932, 2, 0, 0);
    // encli->mtu = 1261;  // 在这设置无效 // ENET_HOST_DEFAULT_MTU=1400
    // m_encli = encli = enet_host_create(NULL, 1, 2, 0, 0);
    qDebug()<<encli<<encli->peerCount<<encli->mtu;
    encli->toxkit = this;
    encli->enet_socket_send = toxenet_socket_send;
    encli->enet_socket_receive = toxenet_socket_receive;
    // encli->compressor.context = (void*)0x1;
    // encli->compressor.compress = enet_simple_compress;
    // encli->compressor.decompress = enet_simple_decompress;
    // encli->compressor.destroy = enet_simple_destroy;
    // enet_host_compress(encli, this->createCompressor());

    m_enpoll->addENetHost(encli);

    int idx = 0;
    foreach (const ToxTunRecord &rec, this->m_cfg->m_recs) {
        QTcpServer *tcpsrv = new QTcpServer();
        QObject::connect(tcpsrv, &QTcpServer::newConnection, this, &Tunnelc::onNewTcpConnection, Qt::QueuedConnection);

        tcpsrv->listen(QHostAddress("127.0.0.1"), rec.m_local_port);
        // tcpsrv->listen(QHostAddress::Any, 7766);
        this->m_tcpsrvs[tcpsrv] = idx++;
    }
}

void Tunnelc::onTunnelConnected()
{
    qDebug()<<"";
    // add friend request
    // 936F45044C8134CD02AA8A97FB4EAC77DA15037DFB7DF9D73C4EC2BFABCF87542D0B8FDBDA5B
    QString hexUserId;
    hexUserId = "936F45044C8134CD02AA8A97FB4EAC77DA15037DFB7DF9D73C4EC2BFABCF87542D0B8FDBDA5B";
    // hexUserId = "DA4515CED8F83788AFFAB6033B4E96B9E3AC01A44FE8CFCA5968B3CF824CAF11786756E91FAB";
    // m_net->addFriend(hexUserId);
}

void Tunnelc::onTunnelDisconnected()
{
    qDebug()<<"";
}


void Tunnelc::onTunnelReadyRead()
{
    qDebug()<<"";

    QHostAddress addr;
    quint16 port;

    // while (m_rudp->hasPendingDatagrams()) {
    //     QByteArray ba = m_rudp->readDatagram(addr, port);
    //     g_peer_sock->write(ba);
    // }
}

void Tunnelc::onToxnetSelfConnectionStatus(int status)
{
    qDebug()<<status;
    if (status == TOX_CONNECTION_NONE) return;
    
    // add friend
    foreach (const ToxTunRecord &rec, this->m_cfg->m_recs) {
        // QString hex_pubkey = "D62E57FCBDC04080C5F78875AB24DB2AE4C797B5C5A9AC69DB5924FD972E363AF2038D5B7A44";
        QString hex_pubkey = rec.m_remote_pubkey;
        QString reqmsg = "from qt cli";
        uint32_t friend_number = m_toxkit->friendAdd(hex_pubkey, reqmsg);
        qDebug()<<friend_number;
        if (friend_number == UINT32_MAX) {
            m_toxkit->friendDelete(hex_pubkey);
            friend_number = m_toxkit->friendAdd(hex_pubkey, reqmsg);
            // friend_number = m_toxkit->friendAddNorequest(hex_pubkey);
            qDebug()<<friend_number;        
        }
    }
    
}

void Tunnelc::onToxnetFriendConnectionStatus(QString pubkey, int status)
{
    qDebug()<<pubkey<<status;

    if (status > TOX_CONNECTION_NONE) {
        QByteArray message("heheheeeeeeeeee");
        // m_toxkit->friendSendMessage(pubkey, message);
    }
}

void Tunnelc::onToxnetFriendMessage(QString pubkey, int type, QByteArray message)
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

    m_inpkts[pubkey].enqueue(message);
}


//////////////////
void Tunnelc::onENetPeerConnected(ENetHost *enhost, ENetPeer *enpeer, quint32 data)
{
    qDebug()<<enhost<<enpeer<<data;
    // ToxTunChannel *chan = this->m_enpeer_chans[enpeer];
    // ToxTunChannel *chan = (ToxTunChannel*)enpeer->toxchan;
    ToxTunChannel *chan = peerLastChan(enpeer);
    // enet_peer_ping_interval()
    // enet_peer_ping(
    enet_peer_timeout(enpeer, UINT32_MAX/*ENET_PEER_TIMEOUT_LIMIT * 0xfffff*/,
                      UINT32_MAX/*ENET_PEER_TIMEOUT_MINIMUM * 0xffff*/,
                      UINT32_MAX/*ENET_PEER_TIMEOUT_MAXIMUM * 0xffff*/);
    enet_peer_ping_interval(enpeer, UINT32_MAX/*ENET_PEER_PING_INTERVAL * 0xfffff*/);
    
    emit chan->m_sock->readyRead();

    if (false) {
        ENetPacket *packet = enet_packet_create("hehehe123", 10, ENET_PACKET_FLAG_RELIABLE);

        enet_packet_resize(packet, 13);
        strcpy((char*)&packet->data[9], "foo");
    
        uint8_t chanid = 0;
        // enet_peer_send(enpeer, chanid, packet);
        m_enpoll->sendPacket(enpeer, chanid, packet);
    }
}

void Tunnelc::onENetPeerDisconnected(ENetHost *enhost, ENetPeer *enpeer)
{
    qDebug()<<enhost<<enpeer<<enpeer->connectID;
    // ToxTunChannel *chan = this->m_enpeer_chans[enpeer];
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

void Tunnelc::onENetPeerPacketReceived(ENetHost *enhost, ENetPeer *enpeer, int chanid, QByteArray packet)
{
    if (packet.length() > 50) {
        // qDebug()<<enhost<<enpeer<<chanid<<packet.length()<<packet.left(20)<<"..."<<packet.right(20);
    } else {
        // qDebug()<<enhost<<enpeer<<chanid<<packet.length()<<packet;
    }

    // ToxTunChannel *chan = this->m_enpeer_chans[enpeer];
    // ToxTunChannel *chan = (ToxTunChannel*)enpeer->toxchan;
    ToxTunChannel *chan = peerLastChan(enpeer);

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
        if (packet == QByteArray("SRVFIN")) {
            if (chan->peer_sock_closed == false) {
                chan->peer_sock_closed = true;
                chan->peer_sock_close_time = QDateTime::currentDateTime();

                this->promiseChannelCleanup(chan);
            } else {
                qDebug()<<"duplicate SRVFIN packet:";
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
void Tunnelc::onNewTcpConnection()
{
    QTcpServer *tcpsrv = (QTcpServer*)sender();
    QTcpSocket *sock = tcpsrv->nextPendingConnection();
    int idx = this->m_tcpsrvs[tcpsrv];
    ToxTunRecord rec = this->m_cfg->m_recs[idx];

    QObject::connect(sock, &QTcpSocket::readyRead, this, &Tunnelc::onTcpReadyRead, Qt::QueuedConnection);
    QObject::connect(sock, &QTcpSocket::disconnected, this, &Tunnelc::onTcpDisconnected, Qt::QueuedConnection);

    //// channels
    QString friendId = "D62E57FCBDC04080C5F78875AB24DB2AE4C797B5C5A9AC69DB5924FD972E363AF2038D5B7A44";
    friendId = rec.m_remote_pubkey;
    ToxTunChannel *chan = new ToxTunChannel();
    chan->m_sock = sock;
    chan->m_enhost = m_encli;
    // chan->m_enpeer = peer;
    chan->m_peer_pubkey = friendId;
    chan->m_host = "127.0.0.1";
    chan->m_port = rec.m_remote_port;
    chan->m_conid = nextConid();

    
    //
    ENetAddress eaddr = {0};
    enet_address_set_host(&eaddr, "127.0.0.1");  // tunip: 10.0.5.x
    eaddr.port = 7767;

    ENetPeer *peer = enet_host_connect(this->m_encli, &eaddr, 2, chan->m_port);
    if (peer == NULL) {
        qDebug()<<"error: can not connect to enet, maybe exceed max connection:"
                <<m_encli->peerCount<<enet_host_used_peer_size(m_encli);
        // cleanup and close socket
        // sock->disconnect();
        // this->m_sock_chans.remove(sock);
        // sock->close();
    }
    qDebug()<<peer<<peer->connectID
            <<"tol:"<<peer->timeoutLimit<<"tomin:"<<peer->timeoutMinimum
            <<"tomax:"<<peer->timeoutMaximum;
    // peer->timeoutLimit *= 10;
    // peer->timeoutMinimum *= 10;
    // peer->timeoutMaximum *= 10;
    enet_peer_timeout(peer, ENET_PEER_TIMEOUT_LIMIT, ENET_PEER_TIMEOUT_MINIMUM, ENET_PEER_TIMEOUT_MAXIMUM);
    enet_peer_ping_interval(peer, ENET_PEER_PING_INTERVAL);

    if (peer->toxchans == NULL) {
        // peer->toxchans = new QVector<ToxTunChannel*>();
    }
    if (peerChansCount(peer) > 0) {
        qDebug()<<peer->incomingPeerID;
        ToxTunChannel *tchan = peerLastChan(peer);
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
        assert(peerChansCount(peer) == 0);
        // assert(1 == 2);
    }
    peerAddChan(peer, chan);
    
    chan->m_enpeer = peer;
    this->m_sock_chans[sock] = chan;
    // this->m_enpeer_chans[peer] = chan;
    this->m_conid_chans[chan->m_conid] = chan;
}

void Tunnelc::onTcpReadyRead()
{
    qDebug()<<"";
    QTcpSocket *sock = (QTcpSocket*)(sender());
    ToxTunChannel *chan = this->m_sock_chans[sock];
    qDebug()<<chan<<chan->m_enpeer->state<<chan->m_enpeer->connectID;

    if (chan->m_enpeer->state == ENET_PEER_STATE_CONNECTED) {
        while (sock->bytesAvailable() > 0) {
            // QByteArray ba = sock->readAll();
            QByteArray ba = sock->read(123);
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
    
    // QByteArray ba = sock->readAll();
    // m_rudp->sendto(ba, "");
}

void Tunnelc::onTcpDisconnected()
{
    qDebug()<<""<<enet_host_used_peer_size(m_encli)<<m_sock_chans.count()<<m_conid_chans.count();
    QTcpSocket *sock = (QTcpSocket*)(sender());
    ToxTunChannel *chan = this->m_sock_chans[sock];
    // qDebug()<<chan<<chan->m_enpeer->state;

    chan->sock_closed = true;
    chan->sock_close_time = QDateTime::currentDateTime();
    
    if (true) {
        QByteArray ba = "CLIFIN";
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


void Tunnelc::promiseChannelCleanup(ToxTunChannel *chan)
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

    ////////////
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
    
    // enpeer->toxchan = NULL; // cleanup
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
    auto later_close_timeout = [enpeer]() {
        qDebug()<<enpeer<<enpeer->state;
        if (enpeer->state != ENET_PEER_STATE_CONNECTED) {
            qDebug()<<"warning, peer currently not connected:"<<enpeer->incomingPeerID;
        }

        if (! (enet_list_empty (& enpeer -> outgoingReliableCommands) &&
               enet_list_empty (& enpeer -> outgoingUnreliableCommands) && 
               enet_list_empty (& enpeer -> sentReliableCommands))) {
            qDebug()<<"warning, maybe has unsent packet:"<<enpeer->incomingPeerID;
            // 这也有可能是ping包啊，真是不好处理
        }

        qDebug()<<"last recv time:"<<enpeer->incomingPeerID
        <<enetx_time_diff(enpeer->lastReceiveTime, enet_time_get());

        qDebug()<<"restore peer timeout, ping interval";
        enet_peer_timeout(enpeer, ENET_PEER_TIMEOUT_LIMIT*2,
                          ENET_PEER_TIMEOUT_MINIMUM*2, ENET_PEER_TIMEOUT_MAXIMUM*2);
        enet_peer_ping_interval(enpeer, ENET_PEER_PING_INTERVAL*2);
            
        // enet_peer_disconnect_now(enpeer, qrand());
        enet_peer_disconnect_later(enpeer, qrand());
    };

    qDebug()<<"last recv time:"<<enpeer->incomingPeerID
            <<enetx_time_diff(enpeer->lastReceiveTime, enet_time_get());
    QTimer::singleShot(5678, later_close_timeout);
}

