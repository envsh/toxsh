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
    void runInlineThread();
    void addENetHost(ENetHost *enhost);
    void removeENetHost(ENetHost *enhost);

public:
    QHash<ENetHost*, int> m_enhosts;
    QTimer *m_timer = NULL;

signals:
    void connected(ENetHost *enhost, ENetPeer *enpeer, quint32 data);
    void disconnected(ENetHost *enhost, ENetPeer *enpeer);
    void packetReceived(ENetHost *enhost, ENetPeer *enpeer, int chanid, QByteArray packet);
};

// some utils function for enet_*
int enet_host_used_peer_size(ENetHost *host);

size_t enet_simple_compress(void * context, const ENetBuffer * inBuffers, size_t inBufferCount, size_t inLimit, enet_uint8 * outData, size_t outLimit);
size_t enet_simple_decompress(void * context, const enet_uint8 * inData, size_t inLimit, enet_uint8 * outData, size_t outLimit);
void enet_simple_destroy(void * context);

#endif /* ENETPOLL_H */