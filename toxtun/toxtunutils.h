#ifndef TOXTUNUTILS_H
#define TOXTUNUTILS_H

#include <QtCore>
#include <QtNetwork>

#include "enet/enet.h"



class ToxTunRecord
{
public:
    QString m_local_host;
    int m_local_port;
    QString m_remote_host;
    int m_remote_port;
    QString m_remote_pubkey;

public:
    void dump();
};

class ToxTunConfig
{
public:
    ToxTunConfig(QString config_file);
    void loadHardCoded();
    void loadConfig();
    
public:
    QString m_config_file;
    QSettings *m_config_sets = NULL;
    QVector<ToxTunRecord> m_recs;
    QString m_srvname;
};


class ToxTunChannel
{
public:
    QTcpSocket *m_sock = NULL;
    QString m_host;
    int m_port;
    QString m_peer_pubkey;
    ENetHost *m_enhost = NULL;
    ENetPeer *m_enpeer = NULL;
    void *m_xenet = NULL;
    uint32_t m_conid = 0;

    //////
    bool sock_closed = false;
    bool enet_closed = false;
};


#endif /* TOXTUNUTILS_H */
