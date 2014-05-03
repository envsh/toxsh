
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
}

void XshCli::onNewBackendConnection()
{
    QTcpSocket *sock = NULL;

    while (m_backend_sock->hasPendingConnections()) {
        sock = m_backend_sock->nextPendingConnection();
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

    
}
