
#include "stunclient.h"
#include "xshcli.h"

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

    // 
    QObject::connect(m_stun_client, &StunClient::allocateDone, this, &XshCli::onAllocateDone);
    QObject::connect(m_stun_client, &StunClient::channelBindDone, this, &XshCli::onChannelBindDone);

    /////
    // myself init
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

    if (cmd == "connect_ok") {
        m_peer_addr = value;
        m_stun_client->allocate();
    }
}

void XshCli::onAllocateDone()
{
    m_stun_client->channelBind(m_peer_addr);
}

void XshCli::onChannelBindDone()
{
    qDebug()<<""<<sender();
    m_channel_done = true;

    Q_ASSERT(m_conn_sock != NULL);
    qint64 abytes = m_conn_sock->bytesAvailable();
    QByteArray ba = m_conn_sock->readAll();

    if (!ba.isEmpty()) {
        m_stun_client->channelData(ba);
    }
}

void XshCli::onNewBackendConnection()
{
    QTcpSocket *sock = NULL;

    while (m_backend_sock->hasPendingConnections()) {
        sock = m_backend_sock->nextPendingConnection();
        m_conn_sock = sock;
        QObject::connect(sock, &QTcpSocket::readyRead, this, &XshCli::onBackendReadyRead);

        QString cmd = QString("connect;xshcli1;xshsrv1;%1").arg(m_mapped_addr);
        m_rly_sock->write(cmd.toLatin1());
        qDebug()<<"accepted:"<<sock<<cmd;
    }
}

void XshCli::onBackendReadyRead()
{
    QTcpSocket *sock = (QTcpSocket*)(sender());
    qDebug()<<sender();

    if (!m_channel_done) {
        return;
    }

    QByteArray ba = sock->readAll();
    m_stun_client->channelData(ba);
}
