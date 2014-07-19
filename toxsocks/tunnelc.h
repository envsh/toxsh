#ifndef _TUNNELC_H_
#define _TUNNELC_H_

#include <QtCore>
#include <QtNetwork>

class ToxNet;
class Srudp;

class Tunnelc : public QObject
{
    Q_OBJECT;
public:
    Tunnelc();
    virtual ~Tunnelc();

    void init();

public:
public slots:
    void onTunnelConnected();
    void onTunnelDisconnected();
    
private slots:
    void onNewConnection();
    void onPeerReadyRead();
    void onPeerDisconnected();

private:
    ToxNet *m_net = NULL;
    Srudp *m_rudp = NULL;

    QTcpServer *m_serv = NULL;
};


#endif /* _TUNNELC_H_ */
