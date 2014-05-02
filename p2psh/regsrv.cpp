#include <stdlib.h>

#include <QtCore>

#include "debugoutput.h"
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

RegSrv::RegSrv()
    : QObject()
{
}

RegSrv::~RegSrv()
{
}

void RegSrv::init()
{
    m_rly_sock = new QTcpSocket();
    QObject::connect(m_rly_sock, &QTcpSocket::connected, this, &RegSrv::onConnected);
    QObject::connect(m_rly_sock, &QTcpSocket::readyRead, this, &RegSrv::onReadyRead);

    m_rly_sock->connectToHost("81.4.106.67", 7890);
}

void RegSrv::onConnected()
{
    qDebug()<<sender();
    QString reg_cmd = "register;regsrv;server;0.0.0.0:0";
    int rc = m_rly_sock->write(reg_cmd.toLatin1());
}

void RegSrv::onReadyRead()
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

    if (cmd == "register") {
        if (m_peers.contains(from)) {
            peer = m_peers.value(from);
        } else {
            peer = new HlpPeer();
            peer->ctime = time(NULL);
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
        
    }
}

