
#include "xshdefs.h"
#include "srudp.h"
#include "stunclient.h"
#include "xshcli.h"


// TODO 也许把前几步的连接做成blocking模式更容易控制些。
XshCli::XshCli()
    : PeerSrv()
{
}

XshCli::~XshCli()
{
}

void XshCli::init()
{
    PeerSrv::init();

    // myself init
    // 
    QObject::connect(m_stun_client, &StunClient::allocateDone, this, &XshCli::onAllocateDone);
    QObject::connect(m_stun_client, &StunClient::createPermissionDone, this, &XshCli::onCreatePermissionDone);
    // QObject::connect(m_stun_client, &StunClient::channelBindDone, this, &XshCli::onChannelBindDone);

    m_rudp = new Srudp(m_stun_client);
    QObject::connect(m_rudp, &Srudp::connected, this, &XshCli::onRudpConnected);
    QObject::connect(m_rudp, &Srudp::connectError, this, &XshCli::onRudpConnectError);
    QObject::connect(m_rudp, &Srudp::readyRead, this, &XshCli::onPacketReadyRead);

    /////
    m_backend_sock = new QTcpServer();
    QObject::connect(m_backend_sock, &QTcpServer::newConnection, this, &XshCli::onNewBackendConnection);
    
    m_backend_sock->listen(QHostAddress("0.0.0.0"), 8222);
}

QString XshCli::getRegCmd()
{
    return QString("register;xshcli1;xshcli;%1").arg(m_mapped_addr);
}

void XshCli::onRelayReadyRead()
{
    qDebug()<<""<<sender();
    QByteArray ba = m_rly_sock->readAll();
    
    QList<QByteArray> elems = ba.split(';');
    qDebug()<<elems;

    QString cmd = elems.at(0);
    QString from = elems.at(1);
    QString to = elems.at(2);
    QString value = elems.at(3);

    if (cmd == "get_peer_info_rsp") {
        m_peer_addr = value;
        // m_peer_relayed_addr = elems.at(4);
        m_stun_client->allocate();
    }

    if (cmd == "connect_ack") {
        // m_channel_done = true;

        // connect rudp first, now
        // qDebug()<<m_peer_relayed_addr<<m_peer_addr;
        // m_rudp->connectToHost(m_peer_relayed_addr.split(':').at(0), m_peer_relayed_addr.split(':').at(1).toUShort());
    }

    if (cmd == "relay_info_rsp") {
        
    }
}

void XshCli::onAllocateDone(QString relayed_addr)
{
    qDebug()<<""<<sender()<<relayed_addr;
    m_relayed_addr = relayed_addr;

    m_stun_client->createPermission(m_peer_addr);


    /*
    QString cmd = QString("relay_info;xshcli1;xshsrv1;%1").arg(relayed_addr);
    m_rly_sock->write(cmd.toLatin1());
    m_rly_sock->waitForBytesWritten();
    */
}

void XshCli::onCreatePermissionDone()
{
    qDebug()<<"";
    // m_perm_done = true;
   
    QString cmd = QString("relay_info_req;xshcli1;xshsrv1;%1").arg(m_relayed_addr);
    m_rly_sock->write(cmd.toLatin1());
    m_rly_sock->waitForBytesWritten();


    if (0) {
        // 试着给peer发送一个连接请求，测试是否能成功。
        // connect rudp first, now
        qDebug()<<m_peer_addr;
        m_rudp->connectToHost(m_peer_addr.split(':').at(0), m_peer_addr.split(':').at(1).toUShort());
    }
}

/*
void XshCli::onChannelBindDone(QString relayed_addr)
{
    qDebug()<<""<<sender()<<relayed_addr;

    QString cmd = QString("relay_info;xshcli1;xshsrv1;%1").arg(relayed_addr);
    m_rly_sock->write(cmd.toLatin1());
    m_rly_sock->waitForBytesWritten();

}
*/

void XshCli::onPacketRecieved(QByteArray pkt)
{
    QByteArray data = pkt;
    qint64 rc = m_conn_sock->write(data);
    qDebug()<<"CBR -> CLI: Write: "<<rc<<pkt.length();
}

void XshCli::onPacketReadyRead()
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

void XshCli::onNewBackendConnection()
{
    QTcpSocket *sock = NULL;

    while (m_backend_sock->hasPendingConnections()) {
        sock = m_backend_sock->nextPendingConnection();
        m_conn_sock = sock;
        QObject::connect(sock, &QTcpSocket::readyRead, this, &XshCli::onBackendReadyRead);

        QString cmd = QString("get_peer_info_req;xshcli1;xshsrv1;%1;%2").arg(m_mapped_addr).arg(m_relayed_addr);
        m_rly_sock->write(cmd.toLatin1());
        qDebug()<<"accepted:"<<sock<<cmd;
    }
}

void XshCli::onBackendReadyRead()
{
    QTcpSocket *sock = (QTcpSocket*)(sender());
    qDebug()<<sender();

    if (!m_perm_done) {
        return;
    }

    QByteArray ba = sock->readAll();
    // m_rudp->sendto(ba, m_peer_relayed_addr);
    // m_rudp->sendto(ba, QString("%1:%2").arg(STUN_SERVER_ADDR).arg(STUN_SERVER_PORT));
}

void XshCli::onBackendDisconnected()
{
    QTcpSocket *sock = (QTcpSocket*)(sender());
    qDebug()<<sender()<<sock->errorString();
}

void XshCli::onRudpConnected()
{
    qDebug()<<""<<sender();

    Q_ASSERT(m_conn_sock != NULL);
    qint64 abytes = m_conn_sock->bytesAvailable();
    QByteArray ba = m_conn_sock->readAll();

    if (!ba.isEmpty()) {
        // m_rudp->sendto(ba, m_peer_relayed_addr);
        m_rudp->sendto(ba, QString("%1:%2").arg(STUN_SERVER_ADDR).arg(STUN_SERVER_PORT));
    }
}

void XshCli::onRudpConnectError()
{
    qDebug()<<""<<sender();    
}

