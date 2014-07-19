
#include "toxnet.h"
#include "srudp.h"

#include "tunneld.h"

Tunneld::Tunneld()
    : QObject(0)
{

}

Tunneld::~Tunneld()
{

}

void Tunneld::init()
{
    m_net = new ToxNet;
    m_rudp = new Srudp(m_net);

    QObject::connect(m_net, &ToxNet::peerConnected, this, &Tunneld::onPeerConnected);
    QObject::connect(m_net, &ToxNet::peerDisconnected, this, &Tunneld::onPeerDisconnected);
    QObject::connect(m_rudp, &Srudp::readyRead, this, &Tunneld::onPeerReadyRead);
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

    QObject::connect(m_sock, &QTcpSocket::readyRead, this, &Tunneld::onDestReadyRead);
    QObject::connect(m_sock, &QTcpSocket::disconnected, this, &Tunneld::onDestDisconnected);

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
}


void Tunneld::onDestConnected()
{

}


void Tunneld::onDestDisconnected()
{

}

void Tunneld::onDestReadyRead()
{

}

