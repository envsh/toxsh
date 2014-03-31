#include <unistd.h>
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

    if (this->m_cached_pkt_seqs.size() > 0) {
        qDebug()<<"cleaning unhandled cached pkt:"<<this->m_cached_pkt_seqs.size();
    }
    this->m_last_pkt_seq = 0;
    this->m_cached_pkt_seqs.clear();
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
    QString str_pkt_id = pkt_id == 0 ? jobj.value("id").toString() : QString("%1").arg(pkt_id);

    QByteArray hex_pkt = jobj.value("pkt").toString().toLocal8Bit();
    QByteArray pkt = QByteArray::fromHex(hex_pkt);

    qDebug()<<"CBR got pkt_id:"<<str_pkt_id<<pkt.length()<<"Bytes";

    if (this->m_conns.size() > 0) {    
        QTcpSocket *sock = (this->m_conns.begin()).key();

        QByteArray ba = "realServerDisconnected-hoho";
        if (ba == pkt) {
            sock->close();
        } else {
            this->cachePacket(jobj);
            qDebug()<<"ready writing to user client..."<<this->m_cached_pkt_seqs.size();
            while (this->m_cached_pkt_seqs.size() > 0) {
                QJsonObject jobj2;
                jobj2 = this->getNextPacket();
                qDebug()<<jobj2.isEmpty();
                if (jobj2.isEmpty()) {
                    qDebug()<<"mass pkt order:"<<jobj.value("seq").toString()
                            <<this->m_last_pkt_seq<<this->m_cached_pkt_seqs.size();
                    sleep(10);
                    break;
                }

                if (this->verifyPacket(jobj)) {
                    this->m_last_pkt_seq += 1;
                } else {
                    this->cachePacket(jobj);
                }
                int rc = sock->write(pkt);
                qDebug()<<"CBR -> CLI: Write: "<<rc;
                assert(rc == pkt.length());

                // qDebug()<<"should write cached pkt:"<<jobj.value("seq").toString();
            }
        }
    }
}

bool VirtTcpD::verifyPacket(QJsonObject jobj)
{
    int seq = jobj.value("seq").toString().toInt();
    if (seq == this->m_last_pkt_seq + 1) {
        return true;
    }
    return false;
}

bool VirtTcpD::cachePacket(QJsonObject jobj)
{
    int seq = jobj.value("seq").toString().toInt();
    this->m_cached_pkt_seqs[seq] = jobj;
    return true;
}

QJsonObject VirtTcpD::getNextPacket()
{
    QJsonObject jobj, jobj2;
    int seq;
    qint64 rmkey;

    QList<qint64> keys = this->m_cached_pkt_seqs.keys();
    for (qint64 key : keys) {
        jobj2 = this->m_cached_pkt_seqs[key];
        seq = jobj2.value("seq").toString().toInt();
        if (seq == this->m_last_pkt_seq+1) {
            rmkey = key;
            jobj = jobj2;
            break;
        }
    }

    if (!jobj.isEmpty()) {
        this->m_cached_pkt_seqs.remove(rmkey);
    }

    return jobj;
}




