
#include "xshdefs.h"
#include "stunclient.h"

#include "srudp.h"
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
    // QObject::connect(m_stun_client, &StunClient::packetRecieved, this, &XshSrv::onPacketRecieved);

    m_stun_client->getMappedAddress();

    m_rudp = new Srudp(m_stun_client);
    QObject::connect(m_rudp, &Srudp::connected, this, &XshSrv::onRudpConnected);
    QObject::connect(m_rudp, &Srudp::connectError, this, &XshSrv::onRudpConnectError);
    QObject::connect(m_rudp, &Srudp::readyRead, this, &XshSrv::onPacketReadyRead);
}

void XshSrv::onMappedAddressRecieved(QString addr)
{
    qDebug()<<addr;
    this->m_mapped_addr = addr;

    m_rly_sock = new QTcpSocket();
    QObject::connect(m_rly_sock, &QTcpSocket::connected, this, &XshSrv::onRelayConnected);
    QObject::connect(m_rly_sock, &QTcpSocket::disconnected, this, &XshSrv::onRelayDisconnected);
    QObject::connect(m_rly_sock, SIGNAL(error(QAbstractSocket::SocketError)),
                     this, SLOT(onRelayError(QAbstractSocket::SocketError)));
    QObject::connect(m_rly_sock, &QTcpSocket::readyRead, this, &XshSrv::onRelayReadyRead);

    m_rly_sock->connectToHost(RELAY_SERVER_ADDR, RELAY_SERVER_PORT);
}

void XshSrv::onAllocateDone(QString relayed_addr)
{
    qDebug()<<sender()<<relayed_addr;

    m_stun_client->createPermission(m_peer_addr);

    QString cmd = QString("relay_info;xshsrv1;xshcli1;%1").arg(relayed_addr);
    qint64 rc = m_rly_sock->write(cmd.toLatin1());

    // m_stun_client->channelBind(m_peer_addr);
}

void XshSrv::onChannelBindDone(QString relayed_addr)
{
    qDebug()<<sender()<<relayed_addr;

    QString cmd = QString("relay_info;xshsrv1;xshcli1;%1").arg(relayed_addr);
    qint64 rc = m_rly_sock->write(cmd.toLatin1());
}

void XshSrv::onPacketRecieved(QByteArray pkt)
{
    QByteArray data = pkt;
    if (!m_backend_sock) {
        qDebug()<<"backend not connecte. cache this packets???";
    } else {
        qint64 rc = m_backend_sock->write(data);
        qDebug()<<"SBR -> SSHD: Write: "<<rc<<pkt.length()
                <<m_backend_sock->errorString()<<m_backend_sock->state();
    }
}

void XshSrv::onPacketReadyRead()
{
    qDebug()<<sender();
    QHostAddress addr;
    quint16 port;
    QByteArray data;

    int cnter = 0;
    while (m_rudp->hasPendingDatagrams()) {
        cnter ++;
        data = m_rudp->readDatagram(addr, port);
        if (data.isEmpty()) {
            qDebug()<<"read empty pkt:";
        } else {
            this->onPacketRecieved(data);
        }
    }
    qDebug()<<"read pkt conut:"<<cnter;    
}

void XshSrv::onRelayConnected()
{
    qDebug()<<sender();
    
    QString reg_cmd = QString("register;xshsrv1;xshsrv;%1").arg(m_mapped_addr);
    qint64 rc = m_rly_sock->write(reg_cmd.toLatin1());
}

void XshSrv::onRelayDisconnected()
{
    QTcpSocket *sock = (QTcpSocket*)(sender());
    qDebug()<<sender()<<sock->errorString();
}

void XshSrv::onRelayError(QAbstractSocket::SocketError error)
{
    QTcpSocket *sock = (QTcpSocket*)(sender());
    qDebug()<<sender()<<sock->errorString();
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
        m_rudp->setHost(m_peer_relayed_addr.split(':').at(0), m_peer_relayed_addr.split(':').at(1).toUShort());
        
        QString to = "xshcli1";
        QString from = "xshsrv1";
        QString cmd_str = QString("connect_ack;%1;%2;%3").arg(from).arg(to).arg(m_peer_relayed_addr);
        qint64 rc = m_rly_sock->write(cmd_str.toLatin1());
        qDebug()<<rc<<cmd_str;

        // QThread::msleep(50);// wait some time for ack arrive to peer

        // connect rudp first now
        // m_rudp->connectToHost(m_peer_relayed_addr.split(':').at(0), m_peer_relayed_addr.split(':').at(1).toUShort());
    }
}

void XshSrv::onBackendConnected()
{
    qDebug()<<""<<sender();
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

    m_rudp->sendto(ba, m_peer_relayed_addr);
}

void XshSrv::onRudpConnected()
{
    qDebug()<<""<<sender();

    m_rudp->serverConnectToHost(m_peer_relayed_addr.split(':').at(0), m_peer_relayed_addr.split(':').at(1).toUShort());

    m_backend_sock = new QTcpSocket();
    QObject::connect(m_backend_sock, &QTcpSocket::connected, this, &XshSrv::onBackendConnected);
    QObject::connect(m_backend_sock, &QTcpSocket::disconnected, this, &XshSrv::onBackendDisconnected);
    QObject::connect(m_backend_sock, &QTcpSocket::readyRead, this, &XshSrv::onBackendReadyRead);
    m_backend_sock->connectToHost("127.0.0.1", 22);
}

void XshSrv::onRudpConnectError()
{
    qDebug()<<""<<sender();    
}

