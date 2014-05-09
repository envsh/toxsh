#include <stdlib.h>

#include <QtCore>

#include "debugoutput.h"
#include "xshdefs.h"
#include "stunclient.h"
#include "regsrv.h"

int main(int argc, char **argv)
{
    qInstallMessageHandler(myMessageOutput);
    QCoreApplication app(argc, argv);

    RegSrv rs;
    rs.init();

    return app.exec();
    return 0;
}


////////////////
///
/////////////////
// TODO relay connect close handle
// TODO relay connection ping keepalive
RegSrv::RegSrv()
    : QObject()
{
}

RegSrv::~RegSrv()
{
}

void RegSrv::init()
{
    m_stun_client = new StunClient(STUN_CLIENT_PORT);
    QObject::connect(m_stun_client, &StunClient::mappedAddressRecieved, this, &RegSrv::onMappedAddressRecieved);

    m_stun_client->getMappedAddress();
}

void RegSrv::onMappedAddressRecieved(QString addr)
{
    qDebug()<<addr;
    this->m_mapped_addr = addr;

    m_rly_sock = new QTcpSocket();
    QObject::connect(m_rly_sock, &QTcpSocket::connected, this, &RegSrv::onRelayConnected);
    QObject::connect(m_rly_sock, &QTcpSocket::disconnected, this, &RegSrv::onRelayDisconnected);
    QObject::connect(m_rly_sock, &QTcpSocket::readyRead, this, &RegSrv::onRelayReadyRead);

    m_rly_sock->connectToHost(RELAY_SERVER_ADDR, RELAY_SERVER_PORT);
}

void RegSrv::onRelayConnected()
{
    qDebug()<<sender();
    QString reg_cmd = "register;regsrv1;regsrv;0.0.0.0:0";
    int rc = m_rly_sock->write(reg_cmd.toLatin1());
}

void RegSrv::onRelayDisconnected()
{
    QTcpSocket *sock = (QTcpSocket*)(sender());
    qDebug()<<sender()<<sock->errorString();
    // TODO retry connect to relay
}

void RegSrv::onRelayReadyRead()
{
    qDebug()<<sender();
    QByteArray ba = m_rly_sock->readAll();
    qDebug()<<ba;

    QList<QByteArray> elems = ba.split(';');
    qDebug()<<elems.size();

    QString cmd = elems.at(0);
    QString from = elems.at(1);
    QString to = elems.at(2);
    QString value = elems.at(3);
    HlpPeer *peer = NULL;
    HlpPeer *from_peer = NULL, *to_peer = NULL;

    if (cmd == "register") {
        if (m_peers.contains(from)) {
            peer = m_peers.value(from);
        } else {
            peer = new HlpPeer();
            peer->ctime = time(NULL);
            m_peers[from] = peer;
        }

        peer->name = from.toStdString();
        peer->type = to.toStdString();

        QStringList tsl = value.split(':');
        peer->ip_addr = tsl.at(0).toStdString();
        peer->ip_port = tsl.at(1).toUShort();
        peer->mtime = time(NULL);

    } else if (cmd == "list_server") {
    } else if (cmd == "list_relay") {
    } else if (cmd == "list_client") {
    } else if (cmd == "get_server") {

    } else if (cmd == "get_client") {
        
    } else if (cmd == "connect") {
        to_peer = this->m_peers.value(to, NULL);
        if (to_peer == NULL) {
            qDebug()<<"can not find dest peer:"<<to;
        }

        from_peer = this->m_peers.value(from, NULL);
        if (from_peer == NULL) {
            qDebug()<<"can not find src peer:"<<from;
        }

        if (to_peer && from_peer) {
            // 发送给 srv
            value = QString("%1:%2").arg(from_peer->ip_addr.c_str()).arg(from_peer->ip_port);
            QString new_cmd_str = QString("connect;%1;%2;%3")
                .arg(from).arg(to).arg(value);
            m_rly_sock->write(new_cmd_str.toLatin1());
            
        }
    } else if (cmd == "relay_info") {
        to_peer = this->m_peers.value(to, NULL);
        if (to_peer == NULL) {
            qDebug()<<"can not find dest peer:"<<to;
        }

        from_peer = this->m_peers.value(from, NULL);
        if (from_peer == NULL) {
            qDebug()<<"can not find src peer:"<<from;
        }

        if (to_peer && from_peer) {
            // 发送给 srv
            QString value2 = QString("%1:%2").arg(from_peer->ip_addr.c_str()).arg(from_peer->ip_port);
            QString new_cmd_str = QString("connect_ok;%1;%2;%3;%4")
                .arg(from).arg(to).arg(value2).arg(value);
            m_rly_sock->write(new_cmd_str.toLatin1());
        }

    } else if (cmd == "connect_ack") {
        m_rly_sock->write(ba);
    }
}

