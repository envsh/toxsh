#ifndef TOXTUNUTILS_H
#define TOXTUNUTILS_H

#include <QtCore>
#include <QtNetwork>

#include "enet/enet.h"

class ToxTunChannel
{
public:
    QTcpSocket *m_sock = NULL;
    QString m_host;
    QString m_port;
    QString m_peer_pubkey;
    ENetHost *m_enhost = NULL;
    ENetPeer *m_enpeer = NULL;
    void *m_xenet = NULL;

    //////
    bool sock_closed = false;
    bool enet_closed = false;
};


#endif /* TOXTUNUTILS_H */
