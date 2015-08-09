#ifndef ENETPOLL_H
#define ENETPOLL_H

#include <QtCore>

#include "enet/enet.h"

class PacketElement
{
public:
    PacketElement(ENetPeer *enpeer, uint8_t chanid, ENetPacket *pkt)
    {
        m_enpeer = enpeer;
        m_chanid = chanid;
        m_pkt = pkt;
    }

    ENetPeer *m_enpeer;
    uint8_t m_chanid;
    ENetPacket *m_pkt;
};

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
    void sendPacket(ENetPeer *enpeer, uint8_t chanid, ENetPacket *pkt);
                     
public slots:
    void testRunThread();
    
public:
    QHash<ENetHost*, int> m_enhosts;
    QTimer *m_timer = NULL;
    QQueue<PacketElement> m_outpkts;
    QMutex m_outpkts_mutex;

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

uint32_t enetx_time_diff(uint32_t oldtime, uint32_t newtime);

#endif /* ENETPOLL_H */
