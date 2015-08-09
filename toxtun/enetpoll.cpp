
#include "enetpoll.h"

// #define ENET_POLL_INTVAL 1000
#define ENET_POLL_INTVAL 50

ENetPoll::ENetPoll(QObject* parent)
    : QThread(parent)
{
    m_timer = new QTimer();
    m_timer->setInterval(ENET_POLL_INTVAL);
    m_timer->setInterval(10);
    QObject::connect(m_timer, &QTimer::timeout, this, &ENetPoll::runInlineThread, Qt::QueuedConnection);
    // m_timer->start();
}

ENetPoll::~ENetPoll()
{}


void ENetPoll::run()
{
    this->runInlineThread();
    // this->exec();
}


void ENetPoll::runInlineThread()
{
    int interval = ENET_POLL_INTVAL;
    ENetEvent event;
    int rc;
    QByteArray packet;
    
    while (true) {
        if (m_enhosts.count() == 0) {
            // break;
            this->msleep(ENET_POLL_INTVAL);
            continue;
        }
        
        ///////////////
        m_outpkts_mutex.lock();
        while (!this->m_outpkts.isEmpty()) {
            PacketElement pe = this->m_outpkts.dequeue();
            enet_peer_send(pe.m_enpeer, pe.m_chanid, pe.m_pkt);
        }
        m_outpkts_mutex.unlock();
        
        ///////////////
        interval = ENET_POLL_INTVAL / m_enhosts.count();
        for (auto it = m_enhosts.begin(); it != m_enhosts.end(); it++) {
            ENetHost *enhost = it.key();
            bool enable = it.value() == 1;
            // rc = enet_host_service(enhost, &event, 1000);
            // rc = enet_host_service(enhost, &event, interval);
            rc = enet_host_service(enhost, &event, 10);
            // qDebug()<<rc;
            if (rc == -1) {
            }

            switch (event.type) {
            case ENET_EVENT_TYPE_CONNECT:
                qDebug()<<"A new client connected from: "
                        <<event.peer->address.host<<event.peer->address.port
                        <<event.data;
                /*
                printf ("A new client connected from %x:%u.\n", 
                        event.peer -> address.host,
                        event.peer -> address.port);
                */
                /* Store any relevant client information here. */
                event.peer -> data = (void*)"Client information";
                emit connected(enhost, event.peer, event.data);
                break;

            case ENET_EVENT_TYPE_RECEIVE:
                /*
                printf ("A packet of length %u containing %s was received from %s on channel %u.\n",
                        event.packet -> dataLength,
                        event.packet -> data,
                        event.peer -> data,
                        event.channelID);
                */
                packet = QByteArray((char*)event.packet->data, event.packet->dataLength);
                
                /* Clean up the packet now that we're done using it. */
                enet_packet_destroy (event.packet);
                emit packetReceived(enhost, event.peer, (int)event.channelID, packet);
                break;
       
            case ENET_EVENT_TYPE_DISCONNECT:
                // printf ("%s disconnected.\n", event.peer -> data);
                qDebug()<<event.peer -> data<<event.data<<event.peer->connectID<<" disconnected.";
                /* Reset the peer's client information. */
                event.peer -> data = NULL;
                emit disconnected(enhost, event.peer);
            }
        }
        // break;
    }


}

void ENetPoll::testRunThread()
{
    qDebug()<<"here";
}

void ENetPoll::addENetHost(ENetHost *enhost)
{
    m_enhosts[enhost] = 1;
}

void ENetPoll::removeENetHost(ENetHost *enhost)
{
    m_enhosts[enhost] = 0;
    m_enhosts.remove(enhost);
}

void ENetPoll::sendPacket(ENetPeer *enpeer, uint8_t chanid, ENetPacket *pkt)
{
    QMutexLocker mtlck(&m_outpkts_mutex);
    PacketElement pe(enpeer, chanid, pkt);
    this->m_outpkts.enqueue(pe);
}

/////////////
int enet_host_used_peer_size(ENetHost *host)
{
    ENetPeer * currentPeer;
    int used = host->peerCount;
    for (currentPeer = host -> peers;
         currentPeer < & host -> peers [host -> peerCount];
         ++ currentPeer)
    {
       if (currentPeer -> state == ENET_PEER_STATE_DISCONNECTED)
           used --;
    }

    return used;
}

// 返回压缩后的大小
size_t enet_simple_compress(void * context, const ENetBuffer * inBuffers, size_t inBufferCount,
                            size_t inLimit, enet_uint8 * outData, size_t outLimit)
{
    return 0;
    if (inLimit < 200) return 0;
    
    qDebug()<<inBufferCount<<inLimit<<outLimit;
    size_t cpsz = 0;
    QByteArray inbuf;
    
    size_t inlen = 0;
    for (int i = 0; i < inBufferCount; i ++) {
        const ENetBuffer *buffer = &inBuffers[i];
        if (inlen + buffer->dataLength > inLimit) {
            break;
        }
        inlen += buffer->dataLength;
        inbuf.append((const char*)buffer->data, buffer->dataLength);
    }

    QByteArray outbuf = qCompress(inbuf, 7);
    memcpy(outData, outbuf.data(), outbuf.length());

    if (outbuf.length() > outLimit) {
        qDebug()<<"invlid compress:"<<outbuf.length()<<outLimit<<inlen;
        return 0;
    }
    return outbuf.length();
    return 0;
}

// 返回解压后的大小
size_t enet_simple_decompress(void * context, const enet_uint8 * inData, size_t inLimit,
                              enet_uint8 * outData, size_t outLimit)
{
    qDebug()<<inLimit<<outLimit;

    QByteArray inbuf((const char*)inData, inLimit);
    QByteArray outbuf = qUncompress(inbuf);

    memcpy(outData, outbuf.data(), outbuf.length());
    return outbuf.length();
    return 0;
}

void enet_simple_destroy(void * context)
{
    qDebug()<<"";
}

uint32_t enetx_time_diff(uint32_t oldtime, uint32_t newtime)
{
    return newtime - oldtime;
}


