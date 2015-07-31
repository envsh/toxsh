#include "toxnet.h"
#include "srudp.h"

#include "tunnelc.h"

Tunnelc::Tunnelc()
    : QObject()
{
}

Tunnelc::~Tunnelc()
{

}

void Tunnelc::init()
{
    m_net = new ToxNet;
    m_rudp = new Srudp(m_net);
    m_rudp->setClientMode(true);

    QObject::connect(m_net, &ToxNet::netConnected, this, &Tunnelc::onTunnelConnected);
    QObject::connect(m_net, &ToxNet::netDisconnected, this, &Tunnelc::onTunnelDisconnected);
    QObject::connect(m_rudp, &Srudp::readyRead, this, &Tunnelc::onTunnelReadyRead);

    m_serv = new QTcpServer;
    QObject::connect(m_serv, &QTcpServer::newConnection, this, &Tunnelc::onNewConnection);
    m_serv->listen(QHostAddress::Any, 3322);
}

void Tunnelc::onTunnelConnected()
{
    qDebug()<<"";
    // add friend request
    // 936F45044C8134CD02AA8A97FB4EAC77DA15037DFB7DF9D73C4EC2BFABCF87542D0B8FDBDA5B
    QString hexUserId;
    hexUserId = "936F45044C8134CD02AA8A97FB4EAC77DA15037DFB7DF9D73C4EC2BFABCF87542D0B8FDBDA5B";
    // hexUserId = "DA4515CED8F83788AFFAB6033B4E96B9E3AC01A44FE8CFCA5968B3CF824CAF11786756E91FAB";
    m_net->addFriend(hexUserId);
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

    while (m_rudp->hasPendingDatagrams()) {
        QByteArray ba = m_rudp->readDatagram(addr, port);
        g_peer_sock->write(ba);
    }
}

void Tunnelc::onNewConnection()
{
    QTcpSocket *sock = m_serv->nextPendingConnection();
    g_peer_sock = sock;

    QObject::connect(sock, &QTcpSocket::readyRead, this, &Tunnelc::onPeerReadyRead);
    QObject::connect(sock, &QTcpSocket::disconnected, this, &Tunnelc::onPeerDisconnected);
}

void Tunnelc::onPeerReadyRead()
{
    qDebug()<<"";
    QTcpSocket *sock = (QTcpSocket*)(sender());
    QByteArray ba = sock->readAll();

    m_rudp->sendto(ba, "");
}

void Tunnelc::onPeerDisconnected()
{
    qDebug()<<"";
}


