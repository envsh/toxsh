
#include "xshdefs.h"
#include "stunclient.h"

#include "xshsrv.h"

XshSrv::XshSrv()
    : QObject()
{
}

XshSrv::~XshSrv()
{
}

void XshSrv::init()
{
    m_stun_client = new StunClient(STUN_CLIENT_PORT_ADD1);
    QObject::connect(m_stun_client, &StunClient::mappedAddressRecieved, this, &XshSrv::onMappedAddressRecieved);
    QObject::connect(m_stun_client, &StunClient::allocateDone, this, &XshSrv::onAllocateDone);
    QObject::connect(m_stun_client, &StunClient::channelBindDone, this, &XshSrv::onChannelBindDone);
    QObject::connect(m_stun_client, &StunClient::packetRecieved, this, &XshSrv::onPacketRecieved);

    m_stun_client->getMappedAddress();
}

void XshSrv::onMappedAddressRecieved(QString addr)
{
    qDebug()<<addr;
    this->m_mapped_addr = addr;

    m_rly_sock = new QTcpSocket();
    QObject::connect(m_rly_sock, &QTcpSocket::connected, this, &XshSrv::onRelayConnected);
    QObject::connect(m_rly_sock, &QTcpSocket::readyRead, this, &XshSrv::onRelayReadyRead);

    m_rly_sock->connectToHost(RELAY_SERVER_ADDR, RELAY_SERVER_PORT);
}

void XshSrv::onAllocateDone()
{
    m_stun_client->channelBind(m_peer_addr);
}

void XshSrv::onChannelBindDone(QString relayed_addr)
{
    qDebug()<<sender()<<relayed_addr;

    QString cmd = QString("relay_info;xshsrv1;xshcli1;%1").arg(relayed_addr);
    qint64 rc = m_rly_sock->write(cmd.toLatin1());
}

void XshSrv::onPacketRecieved(QByteArray pkt)
{
    QByteArray data = QByteArray::fromHex(pkt);
    qint64 rc = m_backend_sock->write(data);
    qDebug()<<rc<<m_backend_sock->errorString()<<QString(pkt).left(60)<<"...";
}

void XshSrv::onRelayConnected()
{
    qDebug()<<sender();
    
    QString reg_cmd = QString("register;xshsrv1;xshsrv;%1").arg(m_mapped_addr);
    qint64 rc = m_rly_sock->write(reg_cmd.toLatin1());
}

void XshSrv::onRelayReadyRead()
{
    qDebug()<<sender();

    QByteArray ba = m_rly_sock->readAll();
    
    QList<QByteArray> elems = ba.split(';');
    qDebug()<<elems;

    QString cmd = elems.at(0);
    QString from = elems.at(1);
    QString to = elems.at(2);
    QString value = elems.at(3);

    if (cmd == "connect") {
        m_peer_addr = value;
        m_stun_client->allocate();
    }

    if (cmd == "connect_ok") {
        m_peer_relayed_addr = elems.at(4);

        m_backend_sock = new QTcpSocket();
        QObject::connect(m_backend_sock, &QTcpSocket::connected, this, &XshSrv::onBackendConnected);
        QObject::connect(m_backend_sock, &QTcpSocket::disconnected, this, &XshSrv::onBackendDisconnected);
        QObject::connect(m_backend_sock, &QTcpSocket::readyRead, this, &XshSrv::onBackendReadyRead);
        m_backend_sock->connectToHost("127.0.0.1", 22);
    }
}

void XshSrv::onBackendConnected()
{
    QString to = "xshcli1";
    QString from = "xshsrv1";
    QString cmd_str = QString("connect_ack;%1;%2;%3").arg(from).arg(to).arg(m_peer_relayed_addr);
    qint64 rc = m_rly_sock->write(cmd_str.toLatin1());
    qDebug()<<rc<<cmd_str;
}

void XshSrv::onBackendDisconnected()
{
    QTcpSocket *sock = (QTcpSocket*)(sender());
    qDebug()<<""<<sender()<<sock->errorString();
}

void XshSrv::onBackendReadyRead()
{
    qDebug()<<sender();
    QByteArray ba = m_backend_sock->readAll();

    m_stun_client->sendRelayData(ba.toHex(), m_peer_relayed_addr);
}

