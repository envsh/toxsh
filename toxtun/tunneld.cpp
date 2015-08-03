
// #include "toxnet.h"
// #include "srudp.h"

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "enet/enet.h"

#include "qtoxkit.h"
#include "serializer.h"

#include "enetpoll.h"
#include "toxtunutils.h"

#include "tunneld.h"



////////////////////////
Tunneld::Tunneld()
    : QObject(0)
{

}

Tunneld::~Tunneld()
{

}

static int toxenet_socket_send (ENetSocket socket, const ENetAddress * address,
                                const ENetBuffer * buffers, size_t bufferCount, void *user_data)
{
    // QToxKit *toxkit = (QToxKit*)user_data;
    Tunneld *tund = (Tunneld*)user_data;
    // qDebug()<<bufferCount<<tund;

    size_t sentLength = 0;
    QString friendId = "451843FCB684FC779B302C08EA33DDB447B61402B075C8AF3E84BEE5FB41C738928615BCDCE4";
    
    QByteArray data = serialize_packet(address, buffers, bufferCount);
    tund->m_toxkit->friendSendMessage(friendId, data);
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
                                  void *user_data)
{
    // QToxKit *toxkit = (QToxKit*)user_data;
    Tunneld *tund = (Tunneld*)user_data;
    // qDebug()<<tund<<socket<<bufferCount;

    if (tund->m_pkts.count() == 0) {
        return 0;
    }

    if (bufferCount != 1) {
        qDebug()<<"not supported >1:" << bufferCount;
    }
    
    struct sockaddr_in sin = {0};
    QString pubkey = tund->m_pkts.begin().key();

    if (tund->m_pkts[pubkey].count() > 0) {
        // qDebug()<<tund<<socket<<bufferCount;
        
        int recvLength = 0;
        QByteArray pkt = tund->m_pkts[pubkey].takeAt(0);
        // deserialize
        recvLength = deserialize_packet(pkt, address, &buffers[0]);
        
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


    m_toxkit = new QToxKit("whtun", true);
    QObject::connect(m_toxkit, &QToxKit::selfConnectionStatus, this,
                     &Tunneld::onToxnetSelfConnectionStatus, Qt::QueuedConnection);
    QObject::connect(m_toxkit, &QToxKit::friendConnectionStatus, this,
                     &Tunneld::onToxnetFriendConnectionStatus, Qt::QueuedConnection);
    QObject::connect(m_toxkit, &QToxKit::friendRequest, this,
                     &Tunneld::onToxnetFriendRequest, Qt::QueuedConnection);
    QObject::connect(m_toxkit, &QToxKit::friendMessage, this,
                     &Tunneld::onToxnetFriendMessage, Qt::QueuedConnection);
    m_toxkit->start();

    m_enpoll = new ENetPoll();
    QObject::connect(m_enpoll, &ENetPoll::connected, this, &Tunneld::onENetPeerConnected);
    QObject::connect(m_enpoll, &ENetPoll::disconnected, this, &Tunneld::onENetPeerDisconnected);
    QObject::connect(m_enpoll, &ENetPoll::packetReceived, this, &Tunneld::onENetPeerPacketReceived);
    m_enpoll->start();

    
    ENetAddress enaddr = {ENET_HOST_ANY, 7767};
    ENetHost *ensrv;

    m_ensrv = ensrv = enet_host_create(&enaddr, 32, 2, 0, 0);
    qDebug()<<ensrv;
    ensrv->toxkit = this;
    ensrv->enet_socket_send = toxenet_socket_send;
    ensrv->enet_socket_receive = toxenet_socket_receive;

    m_enpoll->addENetHost(ensrv);
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

    QObject::connect(m_sock, &QTcpSocket::readyRead, this, &Tunneld::onTcpReadyRead);
    QObject::connect(m_sock, &QTcpSocket::disconnected, this, &Tunneld::onTcpDisconnected);

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

    // put buffers
    if (!m_pkts.contains(pubkey)) {
        m_pkts[pubkey] = QVector<QByteArray>();
    }

    m_pkts[pubkey].append(message);
}

//////////////////
void Tunneld::onENetPeerConnected(ENetHost *enhost, ENetPeer *enpeer)
{
    qDebug()<<enhost<<enpeer;

    QTcpSocket *sock = new QTcpSocket();
    QObject::connect(sock, &QTcpSocket::readyRead, this, &Tunneld::onTcpReadyRead);
    QObject::connect(sock, &QTcpSocket::disconnected, this, &Tunneld::onTcpDisconnected);

    sock->connectToHost("127.0.0.1", 8118);
    sock->waitForConnected();
    
    ToxTunChannel *chan = new ToxTunChannel();
    chan->m_sock = sock;
    chan->m_enpeer = enpeer;
    chan->m_enhost = enhost;

    this->m_sock_chans[sock] = chan;
    this->m_enpeer_chans[enpeer] = chan;
    
}

void Tunneld::onENetPeerDisconnected(ENetHost *enhost, ENetPeer *enpeer)
{
    qDebug()<<enhost<<enpeer;
}

void Tunneld::onENetPeerPacketReceived(ENetHost *enhost, ENetPeer *enpeer, int chanid, QByteArray packet)
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
void Tunneld::onTcpConnected()
{
    QTcpSocket *sock = (QTcpSocket*)sender();    
}


void Tunneld::onTcpDisconnected()
{
    qDebug()<<"";
    QTcpSocket *sock = (QTcpSocket*)sender();
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
            enet_peer_send(enpeer, chanid, packet);
        }
    }

    
    // QByteArray ba = m_sock->readAll();
    // m_rudp->sendto(ba, QString("127.0.0.1:6789"));
}

//////////////////
