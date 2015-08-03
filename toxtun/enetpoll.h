#ifndef ENETPOLL_H
#define ENETPOLL_H

#include <QtCore>

#include "enet/enet.h"

class ENetPoll : public QThread
{
    Q_OBJECT;
public:
    ENetPoll(QObject *parent = 0);
    virtual ~ENetPoll();
    
    void run();
    void addENetHost(ENetHost *enhost);
    void removeENetHost(ENetHost *enhost);

public:
    QHash<ENetHost*, int> m_enhosts;

signals:
    void connected(ENetHost *enhost, ENetPeer *enpeer, quint32 data);
    void disconnected(ENetHost *enhost, ENetPeer *enpeer);
    void packetReceived(ENetHost *enhost, ENetPeer *enpeer, int chanid, QByteArray packet);
};


#endif /* ENETPOLL_H */
