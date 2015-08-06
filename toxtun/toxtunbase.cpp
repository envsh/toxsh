
#include "toxtunutils.h"

#include "toxtunbase.h"

uint32_t ToxTunBase::nextConid()
{
    return this->m_conid ++;
}

/*
  QVector<ToxTunChannel*>* enpeer->toxchans
 */
ToxTunChannel *ToxTunBase::peerLastChan(ENetPeer *enpeer)
{
    QVector<ToxTunChannel*>* chans = (QVector<ToxTunChannel*>*)enpeer->toxchans;
    if (chans == NULL) return NULL;

    int idx = chans->count();
    if (idx == 0) return NULL;
    ToxTunChannel *chan = chans->at(idx-1);
    return chan;
    return 0;
}

int ToxTunBase::peerChansCount(ENetPeer *enpeer)
{
    QVector<ToxTunChannel*>* chans = (QVector<ToxTunChannel*>*)enpeer->toxchans;
    return chans->count();
}

int ToxTunBase::peerRemoveChan(ENetPeer *enpeer, ToxTunChannel *chan)
{
    QVector<ToxTunChannel*>* chans = (QVector<ToxTunChannel*>*)enpeer->toxchans;
    int idx = -1;
    ToxTunChannel *tchan = NULL;
    
    for (int i = 0; i < chans->count(); i++) {
        tchan = chans->at(i);
        if (tchan == chan) {
            idx = i;
            break;
        }
    }

    if (idx ==  -1) {
        qDebug()<<"chan not found:"<<chan<<chan->m_conid;
        return idx;
    }

    chans->remove(idx);
    
    return idx;
}

int ToxTunBase::peerAddChan(ENetPeer *enpeer, ToxTunChannel *chan)
{
    QVector<ToxTunChannel*>* chans = (QVector<ToxTunChannel*>*)enpeer->toxchans;
    chans->append(chan);
    return chans->count();
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
