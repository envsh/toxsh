#include "virttcpd.h"

VirtTcpD::VirtTcpD()
    : QObject(0)
{
}

VirtTcpD::~VirtTcpD()
{
}

void VirtTcpD::init()
{
    this->m_tcpd = new QTcpServer();
    QObject::connect(this->m_tcpd, &QTcpServer::newConnection, this, &VirtTcpD::onNewConnection);

    this->m_tcpd->listen(QHostAddress::Any, 8022);
}

void VirtTcpD::onNewConnection()
{
    QTcpSocket *sock = this->m_tcpd->nextPendingConnection();
    this->m_conns[sock] = sock;

    QObject::connect(sock, &QTcpSocket::disconnected, this, &VirtTcpD::onClientDisconnected);
    QObject::connect(sock, &QTcpSocket::readyRead, this, &VirtTcpD::onClientReadyRead);
}

void VirtTcpD::onClientReadyRead()
{
    QTcpSocket *sock = (QTcpSocket*)(sender());
    QByteArray ba = sock->readAll();

    emit this->newPacket(ba);
}

void VirtTcpD::onClientDisconnected()
{
    QTcpSocket *sock = (QTcpSocket*)(sender());
    this->m_conns.remove(sock);
    sock->deleteLater();
}

