
#include <unistd.h>
#include <assert.h>

#include "virttcpc.h"


VirtTcpC::VirtTcpC()
    : QObject(0)
{
}

VirtTcpC::~VirtTcpC()
{
}

void VirtTcpC::init()
{
    if (!this->m_sock) {
        this->m_sock = new QTcpSocket();
        QObject::connect(this->m_sock, &QTcpSocket::readyRead, this, &VirtTcpC::onClientReadyRead);
        QObject::connect(this->m_sock, &QTcpSocket::disconnected, this, &VirtTcpC::onClientDisconnected);
        QObject::connect(this->m_sock, SIGNAL(error(QAbstractSocket::SocketError)), 
                         this, SLOT(onClientError(QAbstractSocket::SocketError)));
    }
}

void VirtTcpC::connectToHost()
{
    this->m_sock->connectToHost("127.0.0.1", 22);
    this->m_sock->waitForConnected();
}

void VirtTcpC::onClientReadyRead()
{
    QByteArray ba = this->m_sock->readAll();
    qDebug()<<"baa:"<<ba.length();
    emit this->newPacket(ba);
}

void VirtTcpC::onClientDisconnected()
{
    qDebug()<<"real client disconnected";
    QByteArray ba = "realServerDisconnected-hoho";
    emit this->newPacket(ba);
}

void VirtTcpC::onClientError(QAbstractSocket::SocketError error)
{
    QTcpSocket *sock = (QTcpSocket*)(sender());
    qDebug()<<error<<sock->errorString();
}

void VirtTcpC::onPacketRecievedStream(QByteArray pkt)
{
    
}

void VirtTcpC::onPacketRecieved(QJsonObject jobj)
{
    int pkt_id = jobj.value("id").toInt();

    QByteArray hex_pkt = jobj.value("pkt").toString().toLocal8Bit();
    QByteArray pkt = QByteArray::fromHex(hex_pkt);

    qDebug()<<"SBR got pkt_id:"<<pkt_id<<pkt.length();

    QByteArray ba = "realClientDisconnected-hoho";
    if (pkt == ba) {
        this->m_sock->close();
    } else {
        if (this->m_sock->state() == QAbstractSocket::UnconnectedState) {
            this->connectToHost();
        }
        int rc = this->m_sock->write(pkt);
        qDebug()<<"SBR -> SSHD: Write: "<<rc;
        assert(rc == pkt.length());
    }
}

