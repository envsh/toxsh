
#include "xshdefs.h"
#include "stunclient.h"

#include "peersrv.h"

PeerSrv::PeerSrv()
    : QObject()
{
}

PeerSrv::~PeerSrv()
{
}

void PeerSrv::init()
{
    m_stun_client = new StunClient(STUN_CLIENT_PORT_ADD2);
    QObject::connect(m_stun_client, &StunClient::mappedAddressRecieved, this, &PeerSrv::onMappedAddressRecieved);

    m_stun_client->getMappedAddress();
}

void PeerSrv::onMappedAddressRecieved(QString addr)
{
    qDebug()<<addr;
    this->m_mapped_addr = addr;

    m_rly_sock = new QTcpSocket();
    QObject::connect(m_rly_sock, &QTcpSocket::connected, this, &PeerSrv::onRelayConnected);
    QObject::connect(m_rly_sock, &QTcpSocket::disconnected, this, &PeerSrv::onRelayDisconnected);
    QObject::connect(m_rly_sock, SIGNAL(error(QAbstractSocket::SocketError)),
                     this, SLOT(onRelayError(QAbstractSocket::SocketError)));
    QObject::connect(m_rly_sock, &QTcpSocket::readyRead, this, &PeerSrv::onRelayReadyRead);

    m_rly_sock->connectToHost(RELAY_SERVER_ADDR, RELAY_SERVER_PORT);

}

void PeerSrv::onRelayConnected()
{
    qDebug()<<sender();
    QString reg_cmd = this->getRegCmd();

    m_rly_sock->write(reg_cmd.toLatin1());
    // Q_ASSERT(1 == 2);
}

void PeerSrv::onRelayDisconnected()
{
    QTcpSocket *sock = (QTcpSocket*)(sender());
    qDebug()<<sender()<<sock->errorString();
}

void PeerSrv::onRelayError(QAbstractSocket::SocketError error)
{
    QTcpSocket *sock = (QTcpSocket*)(sender());
    qDebug()<<sender()<<sock->errorString();
}


void PeerSrv::onRelayReadyRead()
{
    qDebug()<<sender();
}

