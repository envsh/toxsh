
#include "enetpoll.h"

ENetPoll::ENetPoll(QObject* parent)
    : QThread(parent)
{}

ENetPoll::~ENetPoll()
{}


void ENetPoll::run()
{
    int interval = 1000;
    ENetEvent event;
    int rc;
    QByteArray packet;
    
    while (true) {
        if (m_enhosts.count() == 0) {
            this->msleep(1000);
            continue;
        }
        
        interval = 1000 / m_enhosts.count();
        for (auto it = m_enhosts.begin(); it != m_enhosts.end(); it++) {
            ENetHost *enhost = it.key();
            bool enable = it.value() == 1;
            // rc = enet_host_service(enhost, &event, 1000);
            rc = enet_host_service(enhost, &event, interval);
            // qDebug()<<rc;

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
                qDebug()<<event.peer -> data<<event.data<<" disconnected.";
                /* Reset the peer's client information. */
                event.peer -> data = NULL;
                emit disconnected(enhost, event.peer);
            }
        }
    }

    this->exec();
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

