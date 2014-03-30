#include <assert.h>

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
    QObject::connect(sock, SIGNAL(error(QAbstractSocket::SocketError)), 
                     this, SLOT(onClientError(QAbstractSocket::SocketError)));
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

    QByteArray ba = "realClientDisconnected-hoho";
    emit this->newPacket(ba);
}

void VirtTcpD::onClientError(QAbstractSocket::SocketError error)
{
    QTcpSocket *sock = (QTcpSocket*)(sender());
    qDebug()<<error<<sock->errorString();
}

void VirtTcpD::onPacketRecieved(QJsonObject jobj)
{
    int pkt_id = jobj.value("id").toInt();

    QByteArray hex_pkt = jobj.value("pkt").toString().toLocal8Bit();
    QByteArray pkt = QByteArray::fromHex(hex_pkt);

    qDebug()<<"CBR got pkt_id:"<<pkt_id<<pkt.length();

    if (this->m_conns.size() > 0) {    
        QTcpSocket *sock = (this->m_conns.begin()).key();

        QByteArray ba = "realServerDisconnected-hoho";
        if (ba == pkt) {
            sock->close();
        } else {
            int rc = sock->write(pkt);
            qDebug()<<"CBR -> CLI: Write: "<<rc;
            assert(rc == pkt.length());
        }
    }
}

