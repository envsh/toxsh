// #include "toxnet.h"
// #include "srudp.h"

#include "enet/enet.h"

#include "qtoxkit.h"

#include "tunnelc.h"

////////////////
class Xenet : public QThread
{
public:
    ENetHost *m_enhost = NULL;
    
public:
    
    Xenet(ENetHost *enhost, QObject *parent = NULL)
        : QThread(parent)
    { this->m_enhost = enhost; }

    ~Xenet() {}

    void run()
    {
        ENetEvent event;
        int rc;
        while (true) {
            rc = enet_host_service(this->m_enhost, &event, 1000);
            // qDebug()<<rc;
            switch (event.type) {
            case ENET_EVENT_TYPE_CONNECT:
                printf ("A new client connected from %x:%u.\n", 
                        event.peer -> address.host,
                        event.peer -> address.port);
                /* Store any relevant client information here. */
                event.peer -> data = (void*)"Client information";
                break;
            case ENET_EVENT_TYPE_RECEIVE:
                printf ("A packet of length %u containing %s was received from %s on channel %u.\n",
                        event.packet -> dataLength,
                        event.packet -> data,
                        event.peer -> data,
                        event.channelID);
                /* Clean up the packet now that we're done using it. */
                enet_packet_destroy (event.packet);
        
                break;
       
            case ENET_EVENT_TYPE_DISCONNECT:
                // printf ("%s disconnected.\n", event.peer -> data);
                qDebug()<<event.peer -> data<<" disconnected.";
                /* Reset the peer's client information. */
                event.peer -> data = NULL;
            }
        }
    }
};


////////////////////////

Tunnelc::Tunnelc()
    : QObject()
{
}

Tunnelc::~Tunnelc()
{

}


QByteArray serialize_packet(const ENetAddress * address, const ENetBuffer * buffer)
{
    QByteArray data;
    
    struct msghdr msgHdr;
    struct sockaddr_in sin;
    int sentLength;

    memset (& msgHdr, 0, sizeof (struct msghdr));
    msgHdr = {0};
    sin = {0};
    
    if (address != NULL)
    {
        memset (& sin, 0, sizeof (struct sockaddr_in));

        sin.sin_family = AF_INET;
        sin.sin_port = ENET_HOST_TO_NET_16 (address -> port);
        sin.sin_addr.s_addr = address -> host;

        msgHdr.msg_name = & sin;
        msgHdr.msg_namelen = sizeof (struct sockaddr_in);
    }
    
    data.append((char*)&sin, sizeof(struct sockaddr_in));
    
    data.append((char*)&buffer->dataLength, sizeof(buffer->dataLength));
    data.append((char*)buffer->data, buffer->dataLength);
    qDebug()<<buffer->dataLength;

    return data;
    
    // msgHdr.msg_iov = (struct iovec *) buffers;
    // msgHdr.msg_iovlen = bufferCount;

    // sentLength = sendmsg (socket, & msgHdr, MSG_NOSIGNAL);
    
    // if (sentLength == -1)
    // {
    //    if (errno == EWOULDBLOCK)
    //      return 0;

    //    return -1;
    // }

    // return sentLength;
}


QByteArray serialize_packet(const ENetAddress* address, const ENetBuffer* buffers, size_t bufferCount)
{
    QByteArray data;
    
    struct msghdr msgHdr;
    struct sockaddr_in sin;
    int sentLength = 0;

    memset (& msgHdr, 0, sizeof (struct msghdr));
    msgHdr = {0};
    sin = {0};
    
    if (address != NULL)
    {
        memset (& sin, 0, sizeof (struct sockaddr_in));

        sin.sin_family = AF_INET;
        sin.sin_port = ENET_HOST_TO_NET_16 (address -> port);
        sin.sin_addr.s_addr = address -> host;

        msgHdr.msg_name = & sin;
        msgHdr.msg_namelen = sizeof (struct sockaddr_in);
    }
    
    data.append((char*)&sin, sizeof(struct sockaddr_in));

    for (int i = 0; i < bufferCount; i++) {
        const ENetBuffer *buffer = &buffers[i];
        data.append((char*)&buffer->dataLength, sizeof(buffer->dataLength));
        data.append((char*)buffer->data, buffer->dataLength);
        sentLength += buffer->dataLength;
    }
    qDebug()<<sentLength<<bufferCount;
    if (sentLength > TOX_MAX_MESSAGE_LENGTH) {
        qDebug()<<"warning, exceed tox max message length:"<<sentLength;
    }

    return data;    
}


int toxenet_socket_send (ENetSocket socket, const ENetAddress * address,
                         const ENetBuffer * buffers, size_t bufferCount, void *user_data)
{
    QToxKit *toxkit = (QToxKit*)user_data;
    qDebug()<<bufferCount<<toxkit;

    QByteArray data = serialize_packet(address, buffers, bufferCount);
    toxkit->friendSendMessage(QString(), data);

    // for (int i = 0; i < bufferCount; i++) {
    //     QByteArray data = serialize_packet(address, &buffers[i]);
    //     toxkit->friendSendMessage(QString(), data);
    // }
    
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

    ENetAddress enaddr = {ENET_HOST_ANY, 7766};
    ENetHost *encli;

    m_encli = encli = enet_host_create(&enaddr, 32, 2, 0, 0);
    qDebug()<<encli;
    encli->toxkit = m_toxkit;
    encli->enet_socket_send = toxenet_socket_send;

    Xenet *xloop = new Xenet(encli);
    xloop->start();
    
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
        m_toxkit->friendSendMessage(pubkey, message);
    }
}

void Tunnelc::onToxnetFriendMessage(QString pubkey, int type, QByteArray message)
{
    qDebug()<<pubkey<<type<<message;
}

void Tunnelc::onNewTcpConnection()
{
    QTcpSocket *sock = m_tcpsrv->nextPendingConnection();
    g_peer_sock = sock;

    QObject::connect(sock, &QTcpSocket::readyRead, this, &Tunnelc::onTcpReadyRead);
    QObject::connect(sock, &QTcpSocket::disconnected, this, &Tunnelc::onTcpDisconnected);

    //
    ENetAddress eaddr;
    enet_address_set_host(&eaddr, "10.0.5.5");  // tunip: 10.0.5.x
    eaddr.port = 7766;
    eaddr.toxid = (void*)"";
    
    ENetPeer *peer = enet_host_connect(this->m_encli, &eaddr, 2, 0);
    qDebug()<<peer;
}

void Tunnelc::onTcpReadyRead()
{
    qDebug()<<"";
    QTcpSocket *sock = (QTcpSocket*)(sender());
    QByteArray ba = sock->readAll();

    // m_rudp->sendto(ba, "");
}

void Tunnelc::onTcpDisconnected()
{
    qDebug()<<"";
}


