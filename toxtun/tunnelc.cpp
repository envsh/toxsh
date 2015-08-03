// #include "toxnet.h"
// #include "srudp.h"

#include "enet/enet.h"

#include "qtoxkit.h"
#include "serializer.h"
#include "toxtunutils.h"
#include "enetpoll.h"

#include "tunnelc.h"


////////////////////////

Tunnelc::Tunnelc()
    : QObject()
{
}

Tunnelc::~Tunnelc()
{

}


static int toxenet_socket_send (ENetSocket socket, const ENetAddress * address,
                                const ENetBuffer * buffers, size_t bufferCount, void *user_data)
{
    Tunnelc *tunc = (Tunnelc*)user_data;    
    QToxKit *toxkit = tunc->m_toxkit;
    // qDebug()<<bufferCount<<toxkit;

    size_t sentLength = 0;
    QString friendId = "D62E57FCBDC04080C5F78875AB24DB2AE4C797B5C5A9AC69DB5924FD972E363AF2038D5B7A44";
    
    QByteArray data = serialize_packet(address, buffers, bufferCount);
    toxkit->friendSendMessage(friendId, data);
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
                                  void *user_data)
{
    Tunnelc *tunc = (Tunnelc*)user_data;
    // qDebug()<<tund<<socket<<bufferCount;

    if (tunc->m_pkts.count() == 0) {
        return 0;
    }

    if (bufferCount != 1) {
        qDebug()<<"not supported >1:" << bufferCount;
    }
    
    struct sockaddr_in sin = {0};
    QString pubkey = tunc->m_pkts.begin().key();

    if (tunc->m_pkts[pubkey].count() > 0) {
        // qDebug()<<tunc<<socket<<bufferCount;
        
        int recvLength = 0;
        QByteArray pkt = tunc->m_pkts[pubkey].takeAt(0);
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
    m_toxkit->start();

    m_enpoll = new ENetPoll();
    QObject::connect(m_enpoll, &ENetPoll::connected, this, &Tunnelc::onENetPeerConnected);
    QObject::connect(m_enpoll, &ENetPoll::disconnected, this, &Tunnelc::onENetPeerDisconnected);
    QObject::connect(m_enpoll, &ENetPoll::packetReceived, this, &Tunnelc::onENetPeerPacketReceived);
    m_enpoll->start();

    ENetAddress enaddr = {ENET_HOST_ANY, 7766};
    ENetHost *encli;

    m_encli = encli = enet_host_create(&enaddr, 32, 2, 0, 0);
    qDebug()<<encli;
    encli->toxkit = this;
    encli->enet_socket_send = toxenet_socket_send;
    encli->enet_socket_receive = toxenet_socket_receive;

    m_enpoll->addENetHost(encli);
    
    m_tcpsrv = new QTcpServer();
    QObject::connect(m_tcpsrv, &QTcpServer::newConnection, this, &Tunnelc::onNewTcpConnection);
    m_tcpsrv->listen(QHostAddress::Any, 7766);
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

static QTcpSocket * g_peer_sock = NULL;
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
    QString hex_pubkey = "D62E57FCBDC04080C5F78875AB24DB2AE4C797B5C5A9AC69DB5924FD972E363AF2038D5B7A44";
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

    // put buffers
    if (!m_pkts.contains(pubkey)) {
        m_pkts[pubkey] = QVector<QByteArray>();
    }

    m_pkts[pubkey].append(message);
}


//////////////////
void Tunnelc::onENetPeerConnected(ENetHost *enhost, ENetPeer *enpeer)
{
    qDebug()<<enhost<<enpeer;
    ToxTunChannel *chan = this->m_enpeer_chans[enpeer];
    emit chan->m_sock->readyRead();

    if (false) {
        ENetPacket *packet = enet_packet_create("hehehe123", 10, ENET_PACKET_FLAG_RELIABLE);

        enet_packet_resize(packet, 13);
        strcpy((char*)&packet->data[9], "foo");
    
        uint8_t chanid = 0;
        enet_peer_send(enpeer, chanid, packet);
    }
}

void Tunnelc::onENetPeerDisconnected(ENetHost *enhost, ENetPeer *enpeer)
{
    qDebug()<<enhost<<enpeer;
    ToxTunChannel *chan = this->m_enpeer_chans[enpeer];
}

void Tunnelc::onENetPeerPacketReceived(ENetHost *enhost, ENetPeer *enpeer, int chanid, QByteArray packet)
{
    if (packet.length() > 50) {
        qDebug()<<enhost<<enpeer<<chanid<<packet.length()<<packet.left(30)<<"..."<<packet.right(30);
    } else {
        qDebug()<<enhost<<enpeer<<chanid<<packet.length();
    }

    ToxTunChannel *chan = this->m_enpeer_chans[enpeer];
    QTcpSocket *sock = chan->m_sock;

    int wrlen = sock->write(packet);
}


/////////////////
void Tunnelc::onNewTcpConnection()
{
    QTcpSocket *sock = m_tcpsrv->nextPendingConnection();
    g_peer_sock = sock;

    QObject::connect(sock, &QTcpSocket::readyRead, this, &Tunnelc::onTcpReadyRead);
    QObject::connect(sock, &QTcpSocket::disconnected, this, &Tunnelc::onTcpDisconnected);

    //
    ENetAddress eaddr;
    enet_address_set_host(&eaddr, "127.0.0.1");  // tunip: 10.0.5.x
    eaddr.port = 7767;
    eaddr.toxid = (void*)"";
    
    ENetPeer *peer = enet_host_connect(this->m_encli, &eaddr, 2, 0);
    qDebug()<<peer;

    //// channels
    ToxTunChannel *chan = new ToxTunChannel();
    chan->m_sock = sock;
    chan->m_enhost = m_encli;
    chan->m_enpeer = peer;

    this->m_sock_chans[sock] = chan;
    this->m_enpeer_chans[peer] = chan;
}

void Tunnelc::onTcpReadyRead()
{
    qDebug()<<"";
    QTcpSocket *sock = (QTcpSocket*)(sender());
    ToxTunChannel *chan = this->m_sock_chans[sock];
    qDebug()<<chan<<chan->m_enpeer->state;

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
            enet_peer_send(enpeer, chanid, packet);
        }
    }
    
    // QByteArray ba = sock->readAll();
    // m_rudp->sendto(ba, "");
}

void Tunnelc::onTcpDisconnected()
{
    qDebug()<<"";
}


