
#include "toxtunutils.h"

#include "toxtunbase.h"

uint32_t ToxTunBase::nextConid()
{
    return this->m_conid ++;
}


void ToxTunBase::onToxnetFriendLossyPacket(QString friendId, QByteArray packet)
{
    // qDebug()<<friendId<<packet.length();

    // put buffers
    if (!m_pkts.contains(friendId)) {
        m_pkts[friendId] = QVector<QByteArray>();
    }

    m_pkts[friendId].append(packet);
    // qDebug()<<friendId<<packet.length();
}

void ToxTunBase::onToxnetFriendLosslessPacket(QString friendId, QByteArray packet)
{
    qDebug()<<friendId<<packet.length();
}

void ToxTunBase::promiseCleanupTimeout()
{
    QTimer *timer = (QTimer*)sender();
    QVariant vchan = timer->property("chan");
    ToxTunChannel *chan = (ToxTunChannel*)vchan.value<void*>();
    qDebug()<<timer<<chan<<chan->m_conid;
    this->promiseChannelCleanup(chan);
}
