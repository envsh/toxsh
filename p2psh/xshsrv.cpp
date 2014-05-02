
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

void XshSrv::onRelayConnected()
{
    qDebug()<<sender();
    
    QString reg_cmd = QString("register;xshsrv1;xshsrv;%1").arg(m_mapped_addr);
    qint64 rc = m_rly_sock->write(reg_cmd.toLatin1());
}

void XshSrv::onRelayReadyRead()
{
    qDebug()<<sender();
    
}

void XshSrv::onBackendConnected()
{
}

void XshSrv::onBackendReadyRead()
{

}

