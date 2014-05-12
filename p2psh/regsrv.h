#ifndef _REGSRV_H_
#define _REGSRV_H_


#include <QtCore>
#include <QtNetwork>

class StunClient;

class HlpPeer
{
public:
    std::string type; // server, client, relay
    std::string name;
    std::string ip_addr;
    unsigned short ip_port;
    int cli_fd;
    time_t ctime;
    time_t mtime;
    std::string ip_addr_me; // 对本server的ip
    unsigned short ip_port_me;
};

class RegSrv : public QObject
{
    Q_OBJECT;
public:
    RegSrv();
    virtual ~RegSrv();
    void init();

public slots:
    void onMappedAddressRecieved(QString addr);
    void onRelayConnected();
    void onRelayDisconnected();
    void onRelayError(QAbstractSocket::SocketError error);
    void onRelayReadyRead();

private:
    QString m_mapped_addr;
    StunClient *m_stun_client = NULL;
    QTcpSocket *m_rly_sock = NULL;
    QHash<QString, HlpPeer*> m_peers; // <name,ip:port>
};

#endif /* _REGSRV_H_ */
